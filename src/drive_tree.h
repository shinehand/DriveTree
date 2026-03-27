#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace drivetree {

enum class NodeType {
  Folder,
  File,
};

struct Entry {
  std::string path;
  std::uintmax_t size;
};

struct Node {
  std::string name;
  NodeType type;
  std::uintmax_t size;
  std::vector<Node> children;
};

struct Summary {
  std::uintmax_t total_size;
  std::size_t folder_count;
  std::size_t file_count;
  const Node* largest_child;
};

struct RankedItem {
  std::string path;
  NodeType type;
  std::uintmax_t size;
  double ratio;
};

Node build_tree_from_entries(const std::vector<Entry>& entries, const std::string& fallback_name);
Summary summarize_node(const Node& node);
std::vector<RankedItem> collect_largest_items(const Node& root, std::size_t limit);
std::string format_bytes(std::uintmax_t size);
std::vector<Entry> scan_path(const std::filesystem::path& root_path);
std::vector<Entry> sample_entries();

}  // namespace drivetree
