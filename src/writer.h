#pragma once
#include <filesystem>
#include <string>
#include <utility>

#include "duo_anki_interface.h"
#include "org_formatter.h"
#include "safe_queue.h"
#include "translate_cmd.h"

class Writer {
    std::filesystem::path image_dir{};
    std::filesystem::path org_file_path{};
    void write_to_fs(std::pair<TranslateCommand, DuoAnkiResponse> cmd_resp) const;
    void save_image(const std::string& image_data, const std::string& filename) const;
    const OrgFormatter org_formatter;

public:
    Writer(std::filesystem::path image_dir, std::filesystem::path org_file_path)
        : image_dir{image_dir}
        , org_file_path(org_file_path)
        , org_formatter(org_file_path, image_dir) {}
    void start(SafeQueue<std::pair<TranslateCommand, DuoAnkiResponse>>& queue) const;
};
