#include "writer.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include "duo_anki_interface.h"
#include "org_formatter.h"

std::string sanitize_filename(std::string name) {
    for (char& c : name) {
        if (c == ' ' || c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"'
            || c == '<' || c == '>' || c == '|') {
            c = '_';
        }
    }
    return name;
}

void Writer::write_to_fs(std::pair<TranslateCommand, DuoAnkiResponse> cmd_resp) const {
    auto [cmd, resp] = cmd_resp;
    auto safe_card_name{sanitize_filename(resp.card_name)};
    auto image_path = image_dir.string() + "/" + safe_card_name + ".png";
    auto relative_image_path = image_dir.string() + "/" + resp.translation_date + "/"
                               + safe_card_name;
    save_image(cmd.image_content, image_path);
    org_formatter.append_card(resp, relative_image_path);
    return;
}

void Writer::save_image(const std::string& image_data, const std::string& filename) const {
    try {
        std::ofstream out(filename, std::ios::binary);
        out.write(image_data.c_str(), image_data.size());
        out.close();
    } catch (const std::exception& e) {
        std::cerr << "Error saving image: " << e.what() << std::endl;
    }
}

void Writer::start(SafeQueue<std::pair<TranslateCommand, DuoAnkiResponse>>& queue) const {
    while (true) {
        if (auto cmd_resp = queue.pull()) {
            write_to_fs((*cmd_resp).val);
        }
        std::this_thread::sleep_for(std::chrono::seconds(60));
    }
}
