#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <cstring>
#include <iosfwd>
#include <type_traits>
#include <memory>
#include <limits>

namespace rain_net {
    namespace internal {
        inline constexpr std::size_t MAX_ITEM_SIZE {std::numeric_limits<std::uint16_t>::max()};

        struct MsgHeader final {
            std::uint16_t id {};
            std::uint16_t payload_size {};  // TODO memory layout
        };
    }

    class Connection;

    // Class representing a message, a blob of data
    // Messages can only contain trivially copyable types like numbers, C strings and POD structs
    class Message final {
    public:
        Message() = default;

        explicit Message(std::uint16_t id) {
            header.id = id;
        }

        std::size_t size() const {
            return sizeof(internal::MsgHeader) + header.payload_size;
        }

        std::uint16_t id() const {
            return header.id;
        }

        // Write data to message
        template<typename T>
        Message& operator<<(const T& data) {
            static_assert(
                std::is_trivially_copyable_v<T>,
                "Type must be trivial, like a fundamental data type or a plain-old-data type"
            );
            static_assert(sizeof(T) <= internal::MAX_ITEM_SIZE);

            const std::size_t write_position {payload.size()};

            payload.resize(payload.size() + sizeof(T));
            std::memcpy(payload.data() + write_position, &data, sizeof(T));

            header.payload_size = static_cast<std::uint16_t>(payload.size());

            return *this;
        }

        // Write data from message; reading must be done in reverse
        template<typename T>
        const Message& operator>>(T& data) const {
            static_assert(
                std::is_trivially_copyable_v<T>,
                "Type must be trivial, like a fundamental data type or a plain-old-data type"
            );
            static_assert(sizeof(T) <= internal::MAX_ITEM_SIZE);

            const std::size_t read_position {payload.size() - sizeof(T)};

            std::memcpy(&data, payload.data() + read_position, sizeof(T));
            payload.resize(read_position);

            // Don't reset header.payload_size

            return *this;
        }
    private:
        internal::MsgHeader header;
        mutable std::vector<unsigned char> payload;

        friend std::ostream& operator<<(std::ostream& stream, const Message& message);

        friend class Connection;
    };

    // Print message
    std::ostream& operator<<(std::ostream& stream, const Message& message);

    namespace internal {
        struct OwnedMsg final {
            Message message;
            std::shared_ptr<Connection> remote;
        };
    }
}
