#include "drive_tree.h"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

void build_tree_aggregates_folder_sizes() {
  const auto tree = drivetree::build_tree_from_entries(
      {
          {"Drive/Users/me/video.mov", 300},
          {"Drive/Users/me/archive.zip", 100},
          {"Drive/System/cache.bin", 200},
      },
      "Selected Folder");

  assert(tree.name == "Drive");
  assert(tree.size == 600);
  assert(tree.children.size() == 2);
  assert(tree.children[0].name == "Users");
  assert(tree.children[0].size == 400);
  assert(tree.children[1].name == "System");
  assert(tree.children[1].size == 200);
}

void summarize_node_counts_descendants() {
  const auto tree = drivetree::build_tree_from_entries(
      {
          {"Drive/Documents/a.psd", 400},
          {"Drive/Documents/b.fig", 250},
          {"Drive/Downloads/c.zip", 900},
      },
      "Selected Folder");

  const auto summary = drivetree::summarize_node(tree);

  assert(summary.folder_count == 2);
  assert(summary.file_count == 3);
  assert(summary.largest_child != nullptr);
  assert(summary.largest_child->name == "Downloads");
}

void collect_largest_items_sorts_descendants() {
  const auto tree = drivetree::build_tree_from_entries(
      {
          {"Drive/Users/me/video.mov", 300},
          {"Drive/Users/me/archive.zip", 100},
          {"Drive/System/cache.bin", 200},
      },
      "Selected Folder");

  const auto items = drivetree::collect_largest_items(tree, 3);

  assert(items.size() == 3);
  assert(items[0].path == "Drive/Users");
  assert(items[0].size == 400);
  assert(items[1].path == "Drive/Users/me");
  assert(items[1].size == 400);
  assert(items[2].path == "Drive/Users/me/video.mov");
  assert(items[2].size == 300);
}

void scan_path_reads_real_files() {
  const auto temp_root = std::filesystem::temp_directory_path() / "drivetree_test_scan";
  std::filesystem::remove_all(temp_root);
  std::filesystem::create_directories(temp_root / "nested");

  {
    std::ofstream(temp_root / "nested" / "a.bin") << "hello";
    std::ofstream(temp_root / "b.bin") << "abc";
  }

  const auto entries = drivetree::scan_path(temp_root);
  const auto tree = drivetree::build_tree_from_entries(entries, temp_root.filename().string());
  const auto summary = drivetree::summarize_node(tree);

  assert(entries.size() == 2);
  assert(summary.file_count == 2);
  assert(summary.total_size == 8);

  std::filesystem::remove_all(temp_root);
  assert(!std::filesystem::exists(temp_root));
}

void format_tree_lines_includes_root_and_children() {
  const auto tree = drivetree::build_tree_from_entries(
      {
          {"Drive/Users/me/video.mov", 300},
          {"Drive/System/cache.bin", 200},
      },
      "Selected Folder");

  const auto lines = drivetree::format_tree_lines(tree);

  assert(lines.size() == 6);
  assert(lines[0] == "- [DIR ] Drive  500 B");
  assert(lines[1] == "  - [DIR ] Users  300 B");
  assert(lines[5] == "    - [FILE] cache.bin  200 B");
}

}  // namespace

int main() {
  build_tree_aggregates_folder_sizes();
  summarize_node_counts_descendants();
  collect_largest_items_sorts_descendants();
  scan_path_reads_real_files();
  format_tree_lines_includes_root_and_children();
  std::cout << "All DriveTree tests passed.\n";
  return 0;
}
