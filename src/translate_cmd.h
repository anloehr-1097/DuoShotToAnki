#pragma once
#include <chrono>
#include <string>

struct TranslateCommand {
    std::string image_content;  // image as base64 encoded string
    const std::chrono::time_point<std::chrono::system_clock> x{std::chrono::system_clock::now()};
};
