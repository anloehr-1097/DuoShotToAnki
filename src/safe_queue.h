#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <queue>
#include <unordered_map>

/*
 * Due to the ack, nack functions, make sure that the objects of type T are not pointers.
 * Else when extracting them from unordered_map, the user must make sure to free the memory
 * associated with the pointer.
 */

template <typename T>
class SafeQueue {
private:
    std::queue<T> queue;
    std::mutex mut;
    uint64_t next_id{0};
    std::unordered_map<uint64_t, T> in_process{};

public:
    struct Lease {
        T val;
        uint64_t idx;
    };
    void push(T&& item);
    std::optional<Lease> pull();
    void ack(uint64_t idx);
    void nack(uint64_t idx);
};
