#ifndef GEMINI_CLIENT_H
#define GEMINI_CLIENT_H

#include <cstdio>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>

#include "duo_anki_interface.h"
#include "nlohmann/json_fwd.hpp"
#include "safe_queue.h"
#include "translate_cmd.h"

enum class ClientStatus {
    available,
    rate_limit_exceeded,
};

class GeminiClient {
private:
    const std::string api_key;
    const std::string url;
    FILE* template_file = nullptr;
    mutable std::optional<ClientStatus> status{std::nullopt};
    std::optional<DuoAnkiResponse> handleResponse(nlohmann::json& js) const;

public:
    GeminiClient(const std::string& api_key,
                 const std::string& url,
                 const std::filesystem::path response_template_name = "response_template.json");
    ~GeminiClient();

    std::optional<DuoAnkiResponse> dispatch(const std::string& base64_image,
                                            const std::string& mime_type = "image/png") const;

    void start(SafeQueue<TranslateCommand>& cmd_queue,
               SafeQueue<std::pair<TranslateCommand, DuoAnkiResponse>>& res_queue);
};

#endif  // GEMINI_CLIENT_H
