#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <curl/curl.h>
#include <curl/easy.h>
#include <nlohmann/json.hpp>

#include "config.h"
#include "crow.h"
#include "crow/json.h"
#include "crow/multipart.h"
#include "duo_anki_interface.h"
#include "gemini_client.h"
#include "org_formatter.h"
#include "safe_queue.h"
#include "translate_cmd.h"
#include "writer.h"

constexpr char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

// base64 encoding function (AI generated)
std::string base64_encode(const std::string& in) {
    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) {
        out.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (out.size() % 4) {
        out.push_back('=');
    }
    return out;
}

crow::multipart::part find_image_part(const std::vector<crow::multipart::part>& parts) {
    for (const auto& part : parts) {
        if (part.headers.count("Content-Type") > 0
            && part.headers.find("Content-Type")->second.value.find("image/") == 0) {
            return part;
        }
    }
    throw std::runtime_error("No image part found in multipart message.");
}

int main() {
    crow::SimpleApp app;

    // Load environment variables
    const char* auth_token_env = std::getenv("AUTH_TOKEN");
    const std::string auth_token = auth_token_env ? auth_token_env : "";

    const char* gemini_api_key_env = std::getenv("GEMINI_API_KEY");
    const std::string gemini_api_key = gemini_api_key_env ? gemini_api_key_env : "";

    if (auth_token.empty() || gemini_api_key.empty()) {
        std::cerr << "Warning: AUTH_TOKEN or GEMINI_API_KEY environment variable "
                     "is not set."
                  << std::endl;
    }

    const char* data_dir_env = std::getenv("DATA_DIR");
    const std::string data_dir = data_dir_env ? data_dir_env : ".";

    // TODO(al) these must be fpaths, check if the files / directories exist
    std::string org_file_path = data_dir + "/captured_anki_cards.org";
    std::string images_base_dir = data_dir + "/images";

    GeminiClient gemini_client(gemini_api_key, GEMINI_URL);
    const OrgFormatter org_formatter(org_file_path, images_base_dir);
    SafeQueue<TranslateCommand> cmd_queue{};
    SafeQueue<std::pair<TranslateCommand, DuoAnkiResponse>> res_queue{};

    auto digitizer_thread = std::thread(
        [&gemini_client, &cmd_queue, &res_queue]() { gemini_client.start(cmd_queue, res_queue); });

    auto writer_thread = std::thread([&res_queue, &images_base_dir, &org_file_path]() {
        auto writer = Writer{images_base_dir, org_file_path};
        writer.start(res_queue);
    });

    CROW_ROUTE(app, "/process")
        .methods(crow::HTTPMethod::POST)(
            [&auth_token, &gemini_client, &cmd_queue](const crow::request& req) {
                auto auth_header = req.get_header_value("Authorization");
                if (auth_header.empty() || auth_header != "Bearer " + auth_token) {
                    return crow::response(401, "Unauthorized");
                }

                crow::multipart::message msg(req);
                std::vector<crow::multipart::part> parts{msg.parts};

                try {
                    auto image_part = find_image_part(parts);
                    auto image_content = image_part.body;
                    // Base64 encode the raw binary image data
                    std::string base64_image = base64_encode(image_content);
                    cmd_queue.push(TranslateCommand{image_content});
                    crow::json::wvalue res{};
                    res["status"] = "sucess";
                    return crow::response(200, res);
                } catch (const std::runtime_error& e) {
                    return crow::response(400, "Bad Request: " + std::string(e.what()));
                }
            });

    app.port(8080).multithreaded().run();
}
