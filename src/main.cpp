#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <curl/curl.h>
#include <curl/easy.h>
#include <nlohmann/json.hpp>

#include "config.h"
#include "crow.h"
#include "crow/multipart.h"
#include "duo_anki_interface.h"
#include "gemini_client.h"
#include "org_formatter.h"

constexpr char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

std::string sanitize_filename(std::string name) {
    for (char& c : name) {
        if (c == ' ' || c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"'
            || c == '<' || c == '>' || c == '|') {
            c = '_';
        }
    }
    return name;
}

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

void save_image(const std::string& image_data, const std::string& filename) {
    try {
        std::ofstream out(filename, std::ios::binary);
        out.write(image_data.c_str(), image_data.size());
        out.close();
    } catch (const std::exception& e) {
        std::cerr << "Error saving image: " << e.what() << std::endl;
    }
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

    std::string org_file_path = data_dir + "/captured_anki_cards.org";
    std::string images_base_dir = data_dir + "/images";

    const GeminiClient gemini_client(gemini_api_key, GEMINI_URL);
    const OrgFormatter org_formatter(org_file_path, images_base_dir);

    CROW_ROUTE(app, "/process")
        .methods(crow::HTTPMethod::POST)(
            [&auth_token, &gemini_client, &org_formatter, images_base_dir](
                const crow::request& req) {
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
                    auto translation = gemini_client.dispatch(base64_image);
                    if (translation.card_name.empty() || translation.english.empty()
                        || translation.russian.empty()) {
                        return crow::response(400, "Bad Request: Gemini API data emtpy.");
                    }
                    std::string date_str = get_current_date();
                    std::string image_dir = images_base_dir + "/" + date_str;
                    std::filesystem::create_directories(image_dir);

                    // Use card name for image (sanitize it a bit to avoid spaces/slashes
                    // if needed, or just append extension)
                    std::string safe_card_name = sanitize_filename(translation.card_name);
                    std::string image_path = image_dir + "/" + safe_card_name + ".png";
                    
                    // The path written to the .org file should be relative to the org file itself
                    // Since captured_anki_cards.org and images/ share the same base directory:
                    std::string relative_image_path = "./images/" + date_str + "/" + safe_card_name + ".png";

                    save_image(image_content, image_path);
                    org_formatter.append_card(translation, relative_image_path);

                    crow::json::wvalue res;
                    res["status"] = "success";
                    res["message"] = "Card created successfully with name: " + translation.card_name
                                     + ". English: " + translation.english
                                     + ", Russian: " + translation.russian;
                    return crow::response(200, res);
                } catch (const std::runtime_error& e) {
                    return crow::response(400, "Bad Request: " + std::string(e.what()));
                }
            });

    app.port(8080).multithreaded().run();
}
