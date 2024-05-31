#pragma once

#include <deque>
#include <mutex>
#include <utility>
#include <condition_variable>
#include <cstddef>
#include <chrono>

namespace rain_net {
    namespace internal {
        template<typename T>
        class Queue {
        public:
            ~Queue() = default;

            Queue(const Queue&) = delete;
            Queue& operator=(const Queue&) = delete;
            Queue(Queue&&) = delete;
            Queue& operator=(Queue&&) = delete;

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
                const T item = std::move(queue.back());
                queue.pop_back();
                return item;
            }

            T pop_front() {
                std::lock_guard<std::mutex> lock {mutex};
                const T item = std::move(queue.front());
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
        protected:
            Queue() = default;
        private:
            std::deque<T> queue;
            mutable std::mutex mutex;
        };

        template<typename T>
        class SyncQueue final : public Queue<T> {};

        // template<typename T>
        // class WaitingSyncQueue final : public Queue<T> {
        // public:
        //     void push_back(const T& item) {
        //         Queue<T>::push_back(item);
        //         cv.notify_one();
        //     }

        //     void push_back(T&& item) {
        //         Queue<T>::push_back(std::move(item));
        //         cv.notify_one();
        //     }

        //     void push_front(const T& item) {
        //         Queue<T>::push_front(item);
        //         cv.notify_one();
        //     }

        //     void push_front(T&& item) {
        //         Queue<T>::push_front(std::move(item));
        //         cv.notify_one();
        //     }

        //     void wait() {
        //         using namespace std::chrono_literals;

        //         std::unique_lock<std::mutex> lock {mutex};
        //         cv.wait_for(lock, 3s, [this]() { return !Queue<T>::empty(); });  // TODO
        //     }
        // private:
        //     std::condition_variable cv;
        //     std::mutex mutex;
        // };
    }
}
