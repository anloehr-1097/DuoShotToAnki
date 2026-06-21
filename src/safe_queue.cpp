#include "safe_queue.h"

#include <cstdint>
#include <mutex>
#include <utility>

#include "duo_anki_interface.h"
#include "translate_cmd.h"

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
        return typename SafeQueue<T>::Lease{std::move(item), lease_id};
    }
    return std::nullopt;
}

template <typename T>
void SafeQueue<T>::ack(uint64_t idx) {
    const std::lock_guard<std::mutex> lock(mut);
    if (auto it = in_process.find(idx); it != in_process.end()) {
        in_process.erase(it);
    }
}

/*
 * Given idx which is key in unordered map, requeue the element associated to key.
 */
template <typename T>
void SafeQueue<T>::nack(uint64_t idx) {
    typename std::unordered_map<uint64_t, T>::node_type node;
    {
        const std::lock_guard<std::mutex> lock(mut);
        if (auto it = in_process.find(idx); it != in_process.end()) {
            node = in_process.extract(it);
        }
    }
    if (!node.empty()) {
        push(std::move(node.mapped()));
    }
}

template <typename T>
int SafeQueue<T>::size() {
    const std::lock_guard<std::mutex> lock(mut);
    return queue.size();
}

template class SafeQueue<TranslateCommand>;
template class SafeQueue<std::pair<TranslateCommand, DuoAnkiResponse>>;
