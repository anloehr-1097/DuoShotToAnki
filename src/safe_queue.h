#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <queue>
#include <unordered_map>

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
