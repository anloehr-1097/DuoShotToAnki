#ifndef ORG_FORMATTER_H
#define ORG_FORMATTER_H

#include "duo_anki_interface.h"
#include <filesystem>
#include <string>

class OrgFormatter {
private:
  std::filesystem::path org_file_path;
  std::filesystem::path image_path; // directory to put images
  std::string format_header(const std::string &header, int level = 1) const;
  std::string format_header(const std::string &&header, int level = 1) const;
  std::string add_properties() const;

public:
  OrgFormatter(const std::filesystem::path org_file_path,
               const std::filesystem::path image_path);
  void append_card(const DuoAnkiResponse &card_info,
                   const std::string &image_path) const;
};

std::string get_current_date();
std::string get_current_date_with_day();
#endif // ORG_FORMATTER_H
