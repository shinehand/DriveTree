#include "drive_tree.h"

#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>

namespace {

struct Options {
  std::optional<std::string> path;
  bool sample = false;
  std::size_t limit = 10;
  std::size_t depth = 2;
};

void print_usage() {
  std::cout
      << "Usage: drivetree [--sample] [--limit N] [--depth N] [path]\n"
      << "  --sample    Analyze built-in sample data\n"
      << "  --limit N   Show N largest descendants (default: 10)\n"
      << "  --depth N   Print tree summary up to depth N (default: 2)\n";
}

Options parse_options(int argc, char* argv[]) {
  Options options;

  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];

    if (argument == "--sample") {
      options.sample = true;
      continue;
    }

    if (argument == "--limit" && index + 1 < argc) {
      options.limit = static_cast<std::size_t>(std::stoull(argv[++index]));
      continue;
    }

    if (argument == "--depth" && index + 1 < argc) {
      options.depth = static_cast<std::size_t>(std::stoull(argv[++index]));
      continue;
    }

    if (argument == "--help" || argument == "-h") {
      print_usage();
      std::exit(0);
    }

    if (!argument.empty() && argument[0] == '-') {
      throw std::runtime_error("Unknown option: " + argument);
    }

    options.path = argument;
  }

  if (!options.sample && !options.path.has_value()) {
    throw std::runtime_error("A path is required unless --sample is used.");
  }

  return options;
}

std::string type_label(const drivetree::NodeType type) {
  return type == drivetree::NodeType::Folder ? "DIR " : "FILE";
}

void print_tree(const drivetree::Node& node, std::size_t max_depth, std::size_t current_depth = 0) {
  if (current_depth >= max_depth) {
    return;
  }

  for (const auto& child : node.children) {
    std::cout << std::string(current_depth * 2, ' ') << "- [" << type_label(child.type) << "] " << child.name << "  "
              << drivetree::format_bytes(child.size) << '\n';

    if (child.type == drivetree::NodeType::Folder) {
      print_tree(child, max_depth, current_depth + 1);
    }
  }
}

}  // namespace

int main(int argc, char* argv[]) {
  try {
    const auto options = parse_options(argc, argv);
    const auto entries = options.sample ? drivetree::sample_entries()
                                        : drivetree::scan_path(std::filesystem::path(*options.path));
    const auto display_name = options.sample ? std::string("Sample Drive")
                                             : std::filesystem::path(*options.path).filename().string();
    const auto tree = drivetree::build_tree_from_entries(entries, display_name.empty() ? *options.path : display_name);
    const auto summary = drivetree::summarize_node(tree);
    const auto largest_items = drivetree::collect_largest_items(tree, options.limit);

    std::cout << "DriveTree (C++)\n";
    std::cout << "Target: " << tree.name << "\n";
    std::cout << "Total size: " << drivetree::format_bytes(summary.total_size) << "\n";
    std::cout << "Folders: " << summary.folder_count << "\n";
    std::cout << "Files: " << summary.file_count << "\n";

    if (summary.largest_child != nullptr) {
      std::cout << "Largest direct child: " << summary.largest_child->name << " ("
                << drivetree::format_bytes(summary.largest_child->size) << ")\n";
    }

    std::cout << "\nLargest descendants\n";

    for (const auto& item : largest_items) {
      std::cout << " - [" << type_label(item.type) << "] " << item.path << "  " << drivetree::format_bytes(item.size)
                << "  " << std::fixed << std::setprecision(1) << item.ratio << "%\n";
    }

    std::cout << "\nTree summary (depth " << options.depth << ")\n";
    print_tree(tree, options.depth);
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    print_usage();
    return 1;
  }
}
