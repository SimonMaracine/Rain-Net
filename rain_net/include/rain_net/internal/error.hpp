#pragma once

#include <stdexcept>
#include <string>

namespace rain_net {
    namespace internal {
        struct ConnectionError : public std::runtime_error {
            explicit ConnectionError(const std::string& message)
                : std::runtime_error(message) {}

            explicit ConnectionError(const char* message)
                : std::runtime_error(message) {}
        };
    }
}
