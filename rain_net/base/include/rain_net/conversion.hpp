#pragma once

#include <cstdint>
#include <cstring>
#include <type_traits>
#include <cstddef>
#include <utility>

// https://developer.ibm.com/articles/au-endianc/

namespace rain_net {
    inline bool is_big_endian() noexcept {
        const volatile int i {1};
        return *reinterpret_cast<const volatile unsigned char*>(&i) == 0;
    }

    template<typename T>
    T rev(T x) noexcept {
        static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);
        static_assert(sizeof(T) > 1);

        if (is_big_endian()) {
            return x;
        }

        unsigned char buffer[sizeof(T)];
        std::memcpy(buffer, &x, sizeof(T));

        for (std::size_t i {0}; i < sizeof(buffer); i++) {
            std::swap(buffer[i], buffer[sizeof(buffer) - i - 1]);
        }

        T result;
        std::memcpy(&result, buffer, sizeof(buffer));

        return result;
    }
}
