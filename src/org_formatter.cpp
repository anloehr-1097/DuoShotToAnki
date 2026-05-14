#include "org_formatter.h"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

std::string OrgFormatter::format_header(const std::string &header,
                                        int level) const {
  // lvalue reference -> const reference, no copying
  return std::string(level, '*') + " " + header + "\n";
}

std::string OrgFormatter::format_header(const std::string &&header,
                                        int level) const {
  // move semantics for temporary strings
  return std::string(level, '*') + " " + header + "\n";
}

std::string OrgFormatter::add_properties() const {
  return ":PROPERTIES:\n"
         ":ANKI_DECK: Russian\n"
         ":ANKI_NOTE_TYPE: Basic (and reversed card)\n"
         ":END:\n";
}

OrgFormatter::OrgFormatter(const std::filesystem::path org_file_path,
                           const std::filesystem::path image_path)
    : org_file_path(org_file_path), image_path(image_path) {}

void OrgFormatter::append_card(const DuoAnkiResponse &card,
                               const std::string &image_path) const {

  std::string today_str = get_current_date_with_day();
  // Don't use format_header here because it adds a newline, and we need to
  // match getline
  std::string expected_line = "* " + today_str;
  std::string today_header = expected_line + "\n";
  bool needs_header = true;
  bool is_duplicate = false;

  if (std::filesystem::exists(org_file_path)) {
    std::ifstream in(org_file_path, std::ios::in);
    if (in.fail()) {
      std::cerr << "Failed to open " << org_file_path << " for reading."
                << std::endl;
      return;
    }

    // Read the whole file to check for header and duplicates
    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string content = buffer.str();
    in.close();

    // Check if header exists
    if (content.find(expected_line) != std::string::npos) {
      needs_header = false;
    }

    // Check if this card (by english text) already exists
    std::string english_marker =
        format_header("Front", 3) + card.english + "\n";
    std::string russian_marker = format_header("Back", 3) + card.russian + "\n";

    if (content.find(english_marker + russian_marker) != std::string::npos) {
      is_duplicate = true;
    }
  }

  if (is_duplicate) {
    std::cout << "Duplicate card detected for: " << card.english
              << ". Skipping." << std::endl;
    if (std::filesystem::exists(image_path)) {
      std::filesystem::remove(image_path);
    }
    return;
  }

  // Use a static mutex to prevent concurrent file writes
  static std::mutex file_mutex;
  std::lock_guard<std::mutex> lock(file_mutex);

  std::ofstream out(org_file_path, std::ios::app);
  if (out.fail()) {
    std::cerr << "Failed to open " << org_file_path << " for appending."
              << std::endl;
    return;
  }

  if (needs_header) {
    out << today_header;
  }

  out << format_header(card.card_name, 2) << add_properties()
      << format_header("Front", 3) << card.english << "\n"
      << format_header("Back", 3) << card.russian << "\n"
      << format_header("COMMENT", 3) << "[[" << image_path << "]]" << "\n\n";

  out.close();
}

std::string get_current_date() {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  std::tm tm = *std::localtime(&time);
  std::stringstream ss;
  ss << std::put_time(&tm, "%Y-%m-%d");
  return ss.str();
}

std::string get_current_date_with_day() {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  std::tm tm = *std::localtime(&time);
  std::stringstream ss;
  ss << std::put_time(&tm, "%Y-%m-%d %a");
  return ss.str();
}
