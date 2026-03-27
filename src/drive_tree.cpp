#include "drive_tree.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <unordered_map>

namespace drivetree {
namespace {

struct MutableNode {
  std::string name;
  NodeType type;
  std::uintmax_t size = 0;
  std::vector<MutableNode> children;
  std::unordered_map<std::string, std::size_t> child_indices;
};

std::vector<std::string> split_path(const std::string& raw_path) {
  std::vector<std::string> parts;
  std::string current;

  for (char ch : raw_path) {
    if (ch == '/' || ch == '\\') {
      if (!current.empty()) {
        parts.push_back(current);
        current.clear();
      }
      continue;
    }

    current.push_back(ch);
  }

  if (!current.empty()) {
    parts.push_back(current);
  }

  return parts;
}

MutableNode create_folder(const std::string& name) {
  return MutableNode{name, NodeType::Folder, 0, {}, {}};
}

void insert_entry(MutableNode& root, const std::vector<std::string>& parts, std::uintmax_t size) {
  if (parts.empty()) {
    return;
  }

  MutableNode* current = &root;
  current->size += size;

  for (std::size_t index = 0; index + 1 < parts.size(); ++index) {
    const auto& part = parts[index];
    auto found = current->child_indices.find(part);

    if (found == current->child_indices.end()) {
      current->children.push_back(create_folder(part));
      const std::size_t child_index = current->children.size() - 1;
      current->child_indices.emplace(part, child_index);
      found = current->child_indices.find(part);
    }

    current = &current->children[found->second];
    current->size += size;
  }

  const auto& file_name = parts.back();
  auto found = current->child_indices.find(file_name);

  if (found != current->child_indices.end()) {
    current->children[found->second].size += size;
    return;
  }

  current->children.push_back(MutableNode{file_name, NodeType::File, size, {}, {}});
  current->child_indices.emplace(file_name, current->children.size() - 1);
}

Node finalize_node(MutableNode node) {
  Node final_node{node.name, node.type, node.size, {}};

  if (node.type == NodeType::File) {
    return final_node;
  }

  final_node.children.reserve(node.children.size());

  for (auto& child : node.children) {
    final_node.children.push_back(finalize_node(std::move(child)));
  }

  std::sort(final_node.children.begin(), final_node.children.end(), [](const Node& left, const Node& right) {
    if (left.size != right.size) {
      return left.size > right.size;
    }

    if (left.type != right.type) {
      return left.type == NodeType::Folder;
    }

    return left.name < right.name;
  });

  return final_node;
}

std::pair<std::string, bool> derive_root(const std::vector<Entry>& entries, const std::string& fallback_name) {
  std::optional<std::string> candidate;

  for (const auto& entry : entries) {
    const auto parts = split_path(entry.path);

    if (parts.size() <= 1) {
      return {fallback_name, false};
    }

    if (!candidate.has_value()) {
      candidate = parts.front();
      continue;
    }

    if (*candidate != parts.front()) {
      return {fallback_name, false};
    }
  }

  if (!candidate.has_value()) {
    return {fallback_name, false};
  }

  return {*candidate, true};
}

void collect_descendants(
    const Node& node,
    const std::string& current_path,
    std::uintmax_t root_size,
    std::vector<RankedItem>& items) {
  for (const auto& child : node.children) {
    const auto child_path = current_path.empty() ? child.name : current_path + "/" + child.name;
    const double ratio = root_size == 0 ? 0.0 : static_cast<double>(child.size) * 100.0 / static_cast<double>(root_size);
    items.push_back(RankedItem{child_path, child.type, child.size, ratio});

    if (child.type == NodeType::Folder) {
      collect_descendants(child, child_path, root_size, items);
    }
  }
}

std::string path_name_or_fallback(const std::filesystem::path& path) {
  if (!path.filename().empty()) {
    return path.filename().string();
  }

  return path.root_path().string().empty() ? path.string() : path.root_path().string();
}

}  // namespace

Node build_tree_from_entries(const std::vector<Entry>& entries, const std::string& fallback_name) {
  std::vector<Entry> safe_entries;
  safe_entries.reserve(entries.size());

  for (const auto& entry : entries) {
    safe_entries.push_back(Entry{entry.path, entry.size});
  }

  const auto [root_name, strip_leading_segment] = derive_root(safe_entries, fallback_name);
  MutableNode root = create_folder(root_name);

  for (const auto& entry : safe_entries) {
    auto parts = split_path(entry.path);

    if (strip_leading_segment && !parts.empty()) {
      parts.erase(parts.begin());
    }

    insert_entry(root, parts, entry.size);
  }

  return finalize_node(std::move(root));
}

Summary summarize_node(const Node& node) {
  if (node.type == NodeType::File) {
    return Summary{node.size, 0, 1, &node};
  }

  std::size_t folder_count = 0;
  std::size_t file_count = 0;
  const Node* largest_child = node.children.empty() ? nullptr : &node.children.front();

  for (const auto& child : node.children) {
    if (child.type == NodeType::Folder) {
      ++folder_count;
      const auto child_summary = summarize_node(child);
      folder_count += child_summary.folder_count;
      file_count += child_summary.file_count;
    } else {
      ++file_count;
    }

    if (largest_child == nullptr || child.size > largest_child->size) {
      largest_child = &child;
    }
  }

  return Summary{node.size, folder_count, file_count, largest_child};
}

std::vector<RankedItem> collect_largest_items(const Node& root, std::size_t limit) {
  std::vector<RankedItem> items;
  collect_descendants(root, root.name, root.size, items);

  std::sort(items.begin(), items.end(), [](const RankedItem& left, const RankedItem& right) {
    if (left.size != right.size) {
      return left.size > right.size;
    }

    if (left.type != right.type) {
      return left.type == NodeType::Folder;
    }

    return left.path < right.path;
  });

  if (items.size() > limit) {
    items.resize(limit);
  }

  return items;
}

std::string format_bytes(std::uintmax_t size) {
  if (size == 0) {
    return "0 B";
  }

  static const char* kUnits[] = {"B", "KB", "MB", "GB", "TB", "PB"};
  const double value = static_cast<double>(size);
  const auto exponent = static_cast<std::size_t>(
      std::min<double>(std::floor(std::log(value) / std::log(1024.0)), static_cast<double>(std::size(kUnits) - 1)));
  const double scaled = value / std::pow(1024.0, static_cast<double>(exponent));

  std::ostringstream buffer;
  buffer << std::fixed << std::setprecision((scaled >= 10.0 || exponent == 0) ? 0 : 1) << scaled << ' ' << kUnits[exponent];
  return buffer.str();
}

std::vector<Entry> scan_path(const std::filesystem::path& root_path) {
  std::error_code error;

  if (!std::filesystem::exists(root_path, error) || error) {
    throw std::runtime_error("Path does not exist: " + root_path.string());
  }

  std::vector<Entry> entries;
  const auto display_root = path_name_or_fallback(root_path);

  if (std::filesystem::is_regular_file(root_path, error) && !error) {
    const auto size = std::filesystem::file_size(root_path, error);

    if (error) {
      throw std::runtime_error("Unable to read file size: " + root_path.string());
    }

    entries.push_back(Entry{display_root, size});
    return entries;
  }

  std::filesystem::recursive_directory_iterator iterator(
      root_path, std::filesystem::directory_options::skip_permission_denied, error);
  std::filesystem::recursive_directory_iterator end;

  if (error) {
    throw std::runtime_error("Unable to scan directory: " + root_path.string());
  }

  for (; iterator != end; iterator.increment(error)) {
    if (error) {
      error.clear();
      continue;
    }

    const auto& path = iterator->path();

    if (!iterator->is_regular_file(error) || error) {
      error.clear();
      continue;
    }

    const auto size = iterator->file_size(error);

    if (error) {
      error.clear();
      continue;
    }

    const auto relative = std::filesystem::relative(path, root_path, error);

    if (error) {
      error.clear();
      continue;
    }

    const auto entry_path = (std::filesystem::path(display_root) / relative).generic_string();
    entries.push_back(Entry{entry_path, size});
  }

  return entries;
}

std::vector<Entry> sample_entries() {
  return {
      {"Macintosh HD/Applications/Xcode.app", 14'200'000'000ULL},
      {"Macintosh HD/Applications/Adobe Photoshop.app", 4'100'000'000ULL},
      {"Macintosh HD/Applications/Visual Studio Code.app", 420'000'000ULL},
      {"Macintosh HD/Users/shinehand/Videos/4k-edit.mp4", 8'600'000'000ULL},
      {"Macintosh HD/Users/shinehand/Videos/interview.mov", 3'400'000'000ULL},
      {"Macintosh HD/Users/shinehand/Downloads/dataset.zip", 6'900'000'000ULL},
      {"Macintosh HD/Users/shinehand/Downloads/archive.tar", 2'900'000'000ULL},
      {"Macintosh HD/Users/shinehand/Documents/design.psd", 1'200'000'000ULL},
      {"Macintosh HD/Users/shinehand/Documents/DriveTree/mockups.fig", 860'000'000ULL},
      {"Macintosh HD/Users/shinehand/Documents/DriveTree/export.mov", 4'800'000'000ULL},
      {"Macintosh HD/Users/shinehand/Projects/DriveTree/node_modules.tar", 1'900'000'000ULL},
      {"Macintosh HD/Users/shinehand/Projects/DriveTree/src/app.js", 240'000ULL},
      {"Macintosh HD/System/Library/Caches/cache.data", 2'500'000'000ULL},
      {"Macintosh HD/System/Library/Caches/index.db", 1'100'000'000ULL},
      {"Macintosh HD/System/Library/Fonts/font-pack.pkg", 960'000'000ULL},
  };
}

}  // namespace drivetree
