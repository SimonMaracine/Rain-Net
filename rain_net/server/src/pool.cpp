#include "rain_net/internal/pool.hpp"

#include <cassert>

namespace rain_net {
    namespace internal {
        void Pool::create(std::uint32_t pool_size) {
            m_pool = std::make_unique<bool[]>(pool_size);
            m_size = pool_size;
            m_id_pointer = 0;
        }

        std::optional<std::uint32_t> Pool::allocate_id() {
            std::lock_guard<std::mutex> lock {m_mutex};

            const auto result {search_and_allocate_id(m_id_pointer, m_size)};

            if (result != std::nullopt) {
                return result;
            }

            // No ID found; start searching from the beginning

            return search_and_allocate_id(0, m_id_pointer);

            // Return ID or null, if really nothing found
        }

        void Pool::deallocate_id(std::uint32_t id) {
            std::lock_guard<std::mutex> lock {m_mutex};

            assert(m_pool[id]);

            m_pool[id] = false;
        }

        std::optional<std::uint32_t> Pool::search_and_allocate_id(std::uint32_t begin, std::uint32_t end) noexcept {
            for (std::uint32_t id {begin}; id < end; id++) {
                if (!m_pool[id]) {
                    m_pool[id] = true;
                    m_id_pointer = (id + 1) % m_size;

                    return std::make_optional(id);
                }
            }

            return std::nullopt;
        }
    }
}
