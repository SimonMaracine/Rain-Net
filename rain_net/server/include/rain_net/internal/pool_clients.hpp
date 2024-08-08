#pragma once

#include <cstdint>
#include <optional>
#include <memory>
#include <mutex>

namespace rain_net {
    namespace internal {
        class PoolClients final {
        public:
            PoolClients() noexcept = default;
            ~PoolClients() = default;

            PoolClients(const PoolClients&) = delete;
            PoolClients& operator=(const PoolClients&) = delete;
            PoolClients(PoolClients&& other) = delete;
            PoolClients& operator=(PoolClients&& other) = delete;

            void create_pool(std::uint32_t size);
            std::optional<std::uint32_t> allocate_id();
            void deallocate_id(std::uint32_t id);
        private:
            std::optional<std::uint32_t> search_and_allocate_id(std::uint32_t begin, std::uint32_t end) noexcept;

            std::unique_ptr<bool[]> pool;  // False means it's not allocated
            std::uint32_t size {};
            std::uint32_t id_pointer {};

            std::mutex mutex;
        };
    }
}
