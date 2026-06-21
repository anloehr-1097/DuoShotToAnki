#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "safe_queue.h"
#include "translate_cmd.h"

TEST(SafeQueueTest, TestCreation) {
    auto q = SafeQueue<TranslateCommand>{};
    q.push(TranslateCommand{.image_content = "abc"});
    EXPECT_EQ(q.size(), 1);
}

TEST(SafeQueueTest, TestPull) {
    auto q = SafeQueue<TranslateCommand>{};
    q.push(TranslateCommand{.image_content = "abc"});
    auto elem = q.pull();
    EXPECT_EQ(elem.value().val.image_content, "abc");
}

TEST(SafeQueueTest, TestPullAck) {
    auto q = SafeQueue<TranslateCommand>{};
    q.push(TranslateCommand{.image_content = "abc"});
    auto elem = q.pull();
    q.ack(elem->idx);
    EXPECT_EQ(q.size(), 0);
}

TEST(SafeQueueTest, TestPullNack) {
    auto q = SafeQueue<TranslateCommand>{};
    q.push(TranslateCommand{.image_content = "abc"});
    auto elem = q.pull();
    EXPECT_EQ(q.size(), 0);
    q.nack(elem->idx);
    EXPECT_EQ(q.size(), 1);
}

TEST(SafeQueueTest, TestThreadSafetyPush) {
    auto q = SafeQueue<TranslateCommand>{};
    constexpr int numThreads = 16;
    constexpr int elemsPerThread = 100;
    std::atomic<int> ready_count{0};
    std::atomic<bool> go{false};
    std::vector<std::thread> threads;
    threads.reserve(numThreads);
    for (auto i = 0; i < numThreads; ++i) {
        threads.emplace_back([&q, i, elemsPerThread, &ready_count, &go]() {
            ready_count.fetch_add(1, std::memory_order_relaxed);
            while (!go.load(std::memory_order_acquire)) {}

            for (auto j = 0; j < elemsPerThread; ++j) {
                q.push({.image_content = std::string{"(" + std::to_string(i) + std::to_string(j)
                                                     + ")"}});
            }
        });
    }

    while (ready_count.load(std::memory_order_relaxed) < numThreads) {
        std::this_thread::yield();
    }
    go.store(true, std::memory_order_release);
    for (auto& t : threads) {
        t.join();
    }
    EXPECT_EQ(q.size(), numThreads * elemsPerThread);
}
