FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(test_duolib
    test_org_formatter.cpp
    test_safe_queue.cpp
)

target_link_libraries(test_duolib PRIVATE
    duolib
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_duolib)
