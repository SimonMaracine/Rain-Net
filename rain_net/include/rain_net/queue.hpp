#pragma once

#include <deque>
#include <mutex>
#include <utility>
#include <condition_variable>
#include <cstddef>

namespace rain_net {
    namespace internal {
        template<typename T>
        class Queue {
        public:
            Queue() = default;
            virtual ~Queue() = default;

            Queue(const Queue&) = delete;
            Queue& operator=(const Queue&) = delete;
            Queue(Queue&&) = delete;
            Queue& operator=(Queue&&) = delete;

            virtual void push_back(const T& item) {
                std::lock_guard<std::mutex> lock {queue_mutex};
                queue.push_back(item);
            }

            virtual void push_back(T&& item) {
                std::lock_guard<std::mutex> lock {queue_mutex};
                queue.push_back(std::move(item));
            }

            virtual void push_front(const T& item) {
                std::lock_guard<std::mutex> lock {queue_mutex};
                queue.push_front(item);
            }

            virtual void push_front(T&& item) {
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

            std::size_t size() {
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

        template<typename T>
        class WaitingQueue final : public Queue<T> {
        public:
            WaitingQueue() = default;
            virtual ~WaitingQueue() = default;

            WaitingQueue(const WaitingQueue&) = delete;
            WaitingQueue& operator=(const WaitingQueue&) = delete;
            WaitingQueue(WaitingQueue&&) = delete;
            WaitingQueue& operator=(WaitingQueue&&) = delete;

            virtual void push_back(const T& item) {
                Queue<T>::push_back(item);

                cv.notify_one();
            }

            virtual void push_back(T&& item) {
                Queue<T>::push_back(std::move(item));

                cv.notify_one();
            }

            virtual void push_front(const T& item) {
                Queue<T>::push_front(item);

                cv.notify_one();
            }

            virtual void push_front(T&& item) {
                Queue<T>::push_front(std::move(item));

                cv.notify_one();
            }

            void wait() {
                std::unique_lock<std::mutex> lock {mutex};
                cv.wait(lock, [this]() { return !Queue<T>::empty(); });
            }
        private:
            std::condition_variable cv;
            std::mutex mutex;
        };
    }
}
