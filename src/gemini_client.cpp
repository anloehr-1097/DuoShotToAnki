#include "gemini_client.h"
#include "config.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>

using json = nlohmann::json;

// Callback to read the libcurl response
size_t write_data(char *buffer, size_t size, size_t nmemb, void *userp) {
  size_t total_size = size * nmemb;
  std::string *response = static_cast<std::string *>(userp);
  response->append(buffer, total_size);
  return total_size;
}

GeminiClient::GeminiClient(const std::string &api_key, const std::string &url,
                           const std::filesystem::path response_template_name)
    : api_key(api_key), url(url) {

  // Check if response template file exists
  if (!std::filesystem::exists(response_template_name)) {
    std::cerr << "Warning: Response template file does not exist." << std::endl;
    throw std::runtime_error(
        "Response template file is required for structured output.");
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

DuoAnkiResponse GeminiClient::dispatch(const std::string &base64_image,
                                       const std::string &mime_type) const {
  CURL *handle = curl_easy_init();
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
  } catch (const json::parse_error &e) {
    std::cerr << "Error parsing schema file: " << e.what() << std::endl;
  }
  payload["generationConfig"] = schema_json;
  std::string payload_str = payload.dump();

  // Set headers
  struct curl_slist *headers = NULL;
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
    return DuoAnkiResponse{};
  }

  try {
    json response_json = json::parse(response_buffer);
    // The API returns the content in candidates[0].content.parts[0].text
    std::string content_str =
        response_json["candidates"][0]["content"]["parts"][0]["text"];
    json content_json = json::parse(content_str);

    DuoAnkiResponse resp;
    resp.card_name = content_json.value("card_name", "");
    resp.english = content_json.value("english", "");
    resp.russian = content_json.value("russian", "");
    return resp;
  } catch (const std::exception &e) {
    std::cerr << "JSON Parsing error: " << e.what()
              << "\nResponse buffer: " << response_buffer << std::endl;
    return DuoAnkiResponse{};
  }
}
