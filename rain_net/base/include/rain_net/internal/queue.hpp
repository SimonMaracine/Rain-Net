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
                std::lock_guard<std::mutex> lock {m_mutex};
                m_queue.push_back(item);
            }

            void push_back(T&& item) {
                std::lock_guard<std::mutex> lock {m_mutex};
                m_queue.push_back(std::move(item));
            }

            void push_front(const T& item) {
                std::lock_guard<std::mutex> lock {m_mutex};
                m_queue.push_front(item);
            }

            void push_front(T&& item) {
                std::lock_guard<std::mutex> lock {m_mutex};
                m_queue.push_front(std::move(item));
            }

            T pop_back() {
                std::lock_guard<std::mutex> lock {m_mutex};
                T item {std::move(m_queue.back())};
                m_queue.pop_back();
                return item;
            }

            T pop_front() {
                std::lock_guard<std::mutex> lock {m_mutex};
                T item {std::move(m_queue.front())};
                m_queue.pop_front();
                return item;
            }

            const T& back() const {
                std::lock_guard<std::mutex> lock {m_mutex};
                return m_queue.back();
            }

            const T& front() const {
                std::lock_guard<std::mutex> lock {m_mutex};
                return m_queue.front();
            }

            bool empty() const {
                std::lock_guard<std::mutex> lock {m_mutex};
                return m_queue.empty();
            }

            std::size_t size() const {
                std::lock_guard<std::mutex> lock {m_mutex};
                return m_queue.size();
            }

            void clear() {
                std::lock_guard<std::mutex> lock {m_mutex};
                return m_queue.clear();
            }
        private:
            std::deque<T> m_queue;
            mutable std::mutex m_mutex;
        };
    }
}
