#pragma once

#include <deque>
#include <mutex>
#include <utility>

namespace rain_net {
    template<typename T>
    class Queue final {
    public:
        void push_back(const T& item) {
            std::lock_guard<std::mutex> lock {queue_mutex};
            queue.push_back(item);
        }

        void push_back(T&& item) {
            std::lock_guard<std::mutex> lock {queue_mutex};
            queue.push_back(std::move(item));
        }

        void push_front(const T& item) {
            std::lock_guard<std::mutex> lock {queue_mutex};
            queue.push_front(item);
        }

        void push_front(T&& item) {
            std::lock_guard<std::mutex> lock {queue_mutex};
            queue.push_front(std::move(item));
        }

        T pop_back() {
            std::lock_guard<std::mutex> lock {queue_mutex};
            T item = std::move(queue.back());
            queue.pop_back();
            return item;
        }

        T pop_front() {
            std::lock_guard<std::mutex> lock {queue_mutex};
            T item = std::move(queue.front());
            queue.pop_front();
            return item;
        }

        const T& back() {
            std::lock_guard<std::mutex> lock {queue_mutex};
            return queue.back();
        }

        const T& front() {
            std::lock_guard<std::mutex> lock {queue_mutex};
            return queue.front();
        }

        bool empty() {
            std::lock_guard<std::mutex> lock {queue_mutex};
            return queue.empty();
        }

        size_t size() {
            std::lock_guard<std::mutex> lock {queue_mutex};
            return queue.size();
        }

        void clear() {
            std::lock_guard<std::mutex> lock {queue_mutex};
            return queue.clear();
        }
    private:
        std::deque<T> queue;
        std::mutex queue_mutex;
    };
}
