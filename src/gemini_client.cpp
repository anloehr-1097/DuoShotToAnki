#include "gemini_client.h"

#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#include <curl/curl.h>
#include <curl/easy.h>
#include <nlohmann/json.hpp>

#include "config.h"
#include "duo_anki_interface.h"
#include "org_formatter.h"
#include "safe_queue.h"

using json = nlohmann::json;

// Callback to read the libcurl response
size_t write_data(char* buffer, size_t size, size_t nmemb, void* userp) {
    size_t total_size = size * nmemb;
    std::string* response = static_cast<std::string*>(userp);
    response->append(buffer, total_size);
    return total_size;
}

GeminiClient::GeminiClient(const std::string& api_key,
                           const std::string& url,
                           const std::filesystem::path response_template_name)
    : api_key(api_key), url(url) {
    // Check if response template file exists
    if (!std::filesystem::exists(response_template_name)) {
        std::cerr << "Warning: Response template file does not exist." << std::endl;
        throw std::runtime_error("Response template file is required for structured output.");
    }

    template_file = std::fopen(response_template_name.string().c_str(), "r");
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

GeminiClient::~GeminiClient() {
    curl_global_cleanup();
    if (template_file) {
        std::fclose(template_file);
    }
}

void GeminiClient::start(SafeQueue<TranslateCommand>& cmd_queue,
                         SafeQueue<std::pair<TranslateCommand, DuoAnkiResponse>>& res_queue) {
    while (true) {
        if (status.has_value()) {
            switch (status.value()) {
                case ClientStatus::rate_limit_exceeded:
                    std::cout << "rate limit exceeded, sleeping for 60 seconds";
                    std::this_thread::sleep_for(std::chrono::seconds(60));

                case ClientStatus::available:
                    if (auto tc = cmd_queue.pull()) {
                        auto duo_anki_resp = dispatch((*tc).val.image_content);
                        if (duo_anki_resp) {
                            res_queue.push(
                                std::make_pair(std::move((*tc).val), std::move(*duo_anki_resp)));
                            res_queue.ack(tc->idx);
                        }
                        res_queue.nack(tc->idx);
                    }
            }
        }
    }
}

std::optional<DuoAnkiResponse> GeminiClient::dispatch(const std::string& base64_image,
                                                      const std::string& mime_type) const {
    // TODO(al) make memory safe
    CURL* handle = curl_easy_init();
    if (!handle) {
        throw std::runtime_error("Failed to initialize libcurl");
    }

    std::string response_buffer;

    // Construct URL
    std::string fetch_url{this->url};

    // Build the JSON payload
    json payload;
    payload["contents"][0]["parts"][0]["text"] = INSTRUCTIONS;
    payload["contents"][0]["parts"][1]["inlineData"]["mimeType"] = mime_type;
    payload["contents"][0]["parts"][1]["inlineData"]["data"] = base64_image;

    // Structured Output Configuration
    // Note: fseek is used to rewind the template file so multiple dispatches
    // work.
    std::rewind(template_file);
    json schema_json;
    try {
        schema_json = json::parse(template_file);
    } catch (const json::parse_error& e) {
        std::cerr << "Error parsing schema file: " << e.what() << std::endl;
    }
    payload["generationConfig"] = schema_json;
    std::string payload_str = payload.dump();

    // Set headers
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, ("x-goog-api-key: " + api_key).c_str());

    // Configure cURL
    curl_easy_setopt(handle, CURLOPT_URL, fetch_url.c_str());
    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(handle, CURLOPT_POSTFIELDS, payload_str.c_str());
    curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, payload_str.length());
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &response_buffer);

    auto ccode = curl_easy_perform(handle);

    // Cleanup
    curl_slist_free_all(headers);
    curl_easy_cleanup(handle);

    if (ccode != CURLE_OK) {
        std::cerr << "cURL error: " << curl_easy_strerror(ccode) << std::endl;
        return std::nullopt;
    }

    try {
        json response_json = json::parse(response_buffer);
        auto resp = handleResponse(response_json);
        return resp;

        // std::string content_str = response_json["candidates"][0]["content"]["parts"][0]["text"];
        //         json content_json = json::parse(content_str);
        //
        //         DuoAnkiResponse resp;
        //         resp.card_name = content_json.value("card_name", "");
        //         resp.english = content_json.value("english", "");
        //         resp.russian = content_json.value("russian", "");
        //         resp.translation_date = get_current_date();
        //         return resp;
    } catch (const std::exception& e) {
        std::cerr << "JSON Parsing error: " << e.what() << "\nResponse buffer: " << response_buffer
                  << std::endl;
        return std::nullopt;
    }
}

std::optional<DuoAnkiResponse> GeminiClient::handleResponse(nlohmann::json& js) const {
    if (js.contains("error")) {
        const auto& err = js["error"];
        if (err["code"].is_number_integer()) {
            std::int64_t error_code = err["code"].get<std::int64_t>();

            switch (error_code) {
                case 429:
                    status = ClientStatus::rate_limit_exceeded;
                    break;
                default:
                    break;
            }
        }
        return std::nullopt;
    } else if (js.contains("candidates")) {
        std::string content_str = js["candidates"][0]["content"]["parts"][0]["text"];
        json content_json = json::parse(content_str);
        DuoAnkiResponse resp{.card_name = content_json.value("card_name", ""),
                             .english = content_json.value("english", ""),
                             .russian = content_json.value("russian", ""),
                             .translation_date = get_current_date()};
        return resp;
    } else {
        return std::nullopt;
    }
}
