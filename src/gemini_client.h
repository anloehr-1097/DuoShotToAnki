#ifndef GEMINI_CLIENT_H
#define GEMINI_CLIENT_H

#include <string>
#include <filesystem>
#include <cstdio>
#include "duo_anki_interface.h"

class GeminiClient {
private:
  const std::string api_key;
  const std::string url;
  FILE *template_file = nullptr;

public:
  GeminiClient(const std::string &api_key,
               const std::string &url,
               const std::filesystem::path response_template_name = "response_template.json");
  ~GeminiClient();

  DuoAnkiResponse dispatch(const std::string &base64_image,
                           const std::string &mime_type = "image/png") const;
};

#endif // GEMINI_CLIENT_H