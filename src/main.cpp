#include "drive_tree.h"

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Multiline_Output.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Window.H>
#include <FL/fl_ask.H>

#include <exception>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string type_label(const drivetree::NodeType type) {
  return type == drivetree::NodeType::Folder ? "DIR " : "FILE";
}

std::string summarize_tree(const drivetree::Node& tree) {
  const auto summary = drivetree::summarize_node(tree);
  std::ostringstream buffer;
  buffer << "Target: " << tree.name << '\n'
         << "Total size: " << drivetree::format_bytes(summary.total_size) << '\n'
         << "Folders: " << summary.folder_count << '\n'
         << "Files: " << summary.file_count;

  if (summary.largest_child != nullptr) {
    buffer << "\nLargest direct child: " << summary.largest_child->name << " ("
           << drivetree::format_bytes(summary.largest_child->size) << ")";
  }

  return buffer.str();
}

std::string format_ranked_item(const drivetree::RankedItem& item) {
  std::ostringstream buffer;
  buffer << "[" << type_label(item.type) << "] " << item.path << "  " << drivetree::format_bytes(item.size) << "  "
         << std::fixed << std::setprecision(1) << item.ratio << "%";
  return buffer.str();
}

class DriveTreeWindow : public Fl_Window {
 public:
  DriveTreeWindow()
      : Fl_Window(1120, 790, "DriveTree GUI"),
        path_label_(20, 20, 80, 30, "Path"),
        path_input_(100, 20, 700, 30),
        scan_button_(820, 20, 120, 30, "Scan Folder"),
        sample_button_(960, 20, 140, 30, "Load Sample"),
        tree_label_(20, 70, 300, 25, "Directory Tree"),
        largest_label_(760, 70, 180, 25, "Largest Items"),
        tree_browser_(20, 100, 700, 520),
        largest_browser_(760, 100, 340, 520),
        summary_label_(20, 640, 180, 25, "Summary"),
        summary_output_(20, 670, 1080, 70),
        status_box_(20, 748, 1080, 25, "") {
    path_input_.readonly(1);
    path_input_.value("Load sample data or choose a folder to scan");

    scan_button_.callback(&DriveTreeWindow::on_scan_clicked, this);
    sample_button_.callback(&DriveTreeWindow::on_sample_clicked, this);

    end();
    resizable(tree_browser_);
    load_sample();
  }

 private:
  static constexpr std::size_t kLargestLimit = 20;

  Fl_Box path_label_;
  Fl_Input path_input_;
  Fl_Button scan_button_;
  Fl_Button sample_button_;
  Fl_Box tree_label_;
  Fl_Box largest_label_;
  Fl_Hold_Browser tree_browser_;
  Fl_Hold_Browser largest_browser_;
  Fl_Box summary_label_;
  Fl_Multiline_Output summary_output_;
  Fl_Box status_box_;
  drivetree::Node current_tree_{"", drivetree::NodeType::Folder, 0, {}};
  std::string summary_text_;
  std::string status_text_;
  std::vector<std::string> ranked_lines_;

  static void on_scan_clicked(Fl_Widget*, void* context) {
    static_cast<DriveTreeWindow*>(context)->scan_folder();
  }

  static void on_sample_clicked(Fl_Widget*, void* context) {
    static_cast<DriveTreeWindow*>(context)->load_sample();
  }

  void set_status(const std::string& status) {
    status_text_ = status;
    status_box_.label(status_text_.c_str());
    status_box_.redraw();
    Fl::check();
  }

  void populate_widgets(const std::string& target_display) {
    path_input_.value(target_display.c_str());

    summary_text_ = summarize_tree(current_tree_);
    summary_output_.value(summary_text_.c_str());

    tree_browser_.clear();
    const auto tree_lines = drivetree::format_tree_lines(current_tree_);
    for (const auto& line : tree_lines) {
      tree_browser_.add(line.c_str());
    }
    if (!tree_lines.empty()) {
      tree_browser_.select(1);
      tree_browser_.topline(1);
    }

    largest_browser_.clear();
    ranked_lines_.clear();
    const auto ranked_items = drivetree::collect_largest_items(current_tree_, kLargestLimit);
    for (const auto& item : ranked_items) {
      ranked_lines_.push_back(format_ranked_item(item));
      largest_browser_.add(ranked_lines_.back().c_str());
    }
    if (!ranked_items.empty()) {
      largest_browser_.select(1);
      largest_browser_.topline(1);
    }
  }

  void load_entries(const std::vector<drivetree::Entry>& entries,
                    const std::string& fallback_name,
                    const std::string& target_display,
                    const std::string& status) {
    current_tree_ = drivetree::build_tree_from_entries(entries, fallback_name);
    populate_widgets(target_display);
    set_status(status);
  }

  void load_sample() {
    try {
      set_status("Loading sample data...");
      load_entries(drivetree::sample_entries(), "Sample Drive", "Sample Drive", "Loaded sample data.");
    } catch (const std::exception& error) {
      fl_alert("Failed to load sample data:\n%s", error.what());
      set_status("Failed to load sample data.");
    }
  }

  void scan_folder() {
    Fl_Native_File_Chooser chooser;
    chooser.title("Choose a folder to scan");
    chooser.type(Fl_Native_File_Chooser::BROWSE_DIRECTORY);

    const int result = chooser.show();
    if (result == 1) {
      set_status("Folder selection canceled.");
      return;
    }

    if (result == -1) {
      fl_alert("Unable to open folder chooser:\n%s", chooser.errmsg());
      set_status("Folder chooser failed.");
      return;
    }

    try {
      const std::filesystem::path path(chooser.filename());
      const std::string target_display = path.string();
      std::string fallback_name = path.filename().string();
      if (fallback_name.empty()) {
        fallback_name = target_display;
      }

      set_status("Scanning folder...");
      load_entries(drivetree::scan_path(path), fallback_name, target_display, "Scan completed.");
    } catch (const std::exception& error) {
      fl_alert("Scan failed:\n%s", error.what());
      set_status("Scan failed.");
    }
  }
};

}  // namespace

int main(int argc, char* argv[]) {
  Fl::scheme("gtk+");
  DriveTreeWindow window;
  window.show(argc, argv);
  return Fl::run();
}
