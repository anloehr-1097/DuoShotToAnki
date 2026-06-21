#include "safe_queue.h"

#include <cstdint>
#include <mutex>
#include <utility>

template <typename T>
void SafeQueue<T>::push(T&& item) {
    const std::lock_guard<std::mutex> lock(mut);
    queue.push(item);
}

template <typename T>
std::optional<typename SafeQueue<T>::Lease> SafeQueue<T>::pull() {
    const std::lock_guard<std::mutex> lock(mut);
    if (!queue.empty()) {
        T item{std::move(queue.front())};
        queue.pop();
        auto lease_id = next_id++;
        in_process.emplace(lease_id, item);
        return typename SafeQueue<T>::Lease{std::move(item), next_id};
    }
    return std::nullopt;
}

template <typename T>
void SafeQueue<T>::ack(uint64_t idx) {
    if (auto elem = in_process.find(idx) != in_process.end()) {
        in_process.erase(elem);
    }
    return;
}

/*
 * Given idx which is key in unordered map, requeue the element associated to key.
 */
template <typename T>
void SafeQueue<T>::nack(uint64_t idx) {
    if (auto it_to_elem = in_process.find(idx) != in_process.end()) {
        auto node = in_process.extract(it_to_elem);
        push(std::move(node.mapped()));
    }
    return;
}
