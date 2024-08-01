#pragma once

#include <deque>
#include <mutex>
#include <utility>
#include <cstddef>

namespace rain_net {
    namespace internal {
        template<typename T>
        class SyncQueue {
        public:
            SyncQueue() = default;
            ~SyncQueue() = default;

            SyncQueue(const SyncQueue&) = delete;
            SyncQueue& operator=(const SyncQueue&) = delete;
            SyncQueue(SyncQueue&&) = delete;
            SyncQueue& operator=(SyncQueue&&) = delete;

            void push_back(const T& item) {
                std::lock_guard<std::mutex> lock {mutex};
                queue.push_back(item);
            }

            void push_back(T&& item) {
                std::lock_guard<std::mutex> lock {mutex};
                queue.push_back(std::move(item));
            }

            void push_front(const T& item) {
                std::lock_guard<std::mutex> lock {mutex};
                queue.push_front(item);
            }

            void push_front(T&& item) {
                std::lock_guard<std::mutex> lock {mutex};
                queue.push_front(std::move(item));
            }

            T pop_back() {
                std::lock_guard<std::mutex> lock {mutex};
                const T item {std::move(queue.back())};
                queue.pop_back();
                return item;
            }

            T pop_front() {
                std::lock_guard<std::mutex> lock {mutex};
                const T item {std::move(queue.front())};
                queue.pop_front();
                return item;
            }

            const T& back() const {
                std::lock_guard<std::mutex> lock {mutex};
                return queue.back();
            }

            const T& front() const {
                std::lock_guard<std::mutex> lock {mutex};
                return queue.front();
            }

            bool empty() const {
                std::lock_guard<std::mutex> lock {mutex};
                return queue.empty();
            }

            std::size_t size() const {
                std::lock_guard<std::mutex> lock {mutex};
                return queue.size();
            }

            void clear() {
                std::lock_guard<std::mutex> lock {mutex};
                return queue.clear();
            }
        private:
            std::deque<T> queue;
            mutable std::mutex mutex;
        };
    }
}
