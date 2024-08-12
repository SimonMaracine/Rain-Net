#pragma once

#include <cstdint>
#include <optional>
#include <memory>
#include <mutex>

namespace rain_net {
    namespace internal {
        class Pool final {
        public:
            Pool() noexcept = default;
            ~Pool() = default;

            Pool(const Pool&) = delete;
            Pool& operator=(const Pool&) = delete;
            Pool(Pool&& other) = delete;
            Pool& operator=(Pool&& other) = delete;

            void create(std::uint32_t size);
            std::optional<std::uint32_t> allocate_id();
            void deallocate_id(std::uint32_t id);
        private:
            std::optional<std::uint32_t> search_and_allocate_id(std::uint32_t begin, std::uint32_t end) noexcept;

            std::unique_ptr<bool[]> m_pool;  // False means it's not allocated
            std::uint32_t m_size {};
            std::uint32_t m_id_pointer {};

            std::mutex m_mutex;
        };
    }
}
