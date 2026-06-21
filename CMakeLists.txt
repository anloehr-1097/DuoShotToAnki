cmake_minimum_required(VERSION 3.14)
project(DuoShotToAnkiCard CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Default to Debug build (adds -g flag)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TESTING "Build tests" OFF)

include(FetchContent)

# Fetch ASIO (Dependency for Crow)
FetchContent_Declare(
  asio
  GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
  GIT_TAG asio-1-30-2
)
FetchContent_MakeAvailable(asio)
set(ASIO_INCLUDE_DIR "${asio_SOURCE_DIR}/asio/include")

# Fetch Crow
FetchContent_Declare(
  Crow
  GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
  GIT_TAG v1.2.0
)
FetchContent_MakeAvailable(Crow)

# Fetch nlohmann/json
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# Find cURL
find_package(CURL REQUIRED)

add_library(duolib STATIC
    src/gemini_client.cpp
    src/org_formatter.cpp
    src/safe_queue.cpp
    src/writer.cpp
)

target_include_directories(duolib PUBLIC src)
target_link_libraries(duolib PUBLIC
    Crow::Crow
    nlohmann_json::nlohmann_json
    CURL::libcurl
)

add_executable(server src/main.cpp)
target_link_libraries(server PRIVATE duolib)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()
