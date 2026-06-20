#include "safe_queue.h"

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

// TODO(al) implement ack and nack
