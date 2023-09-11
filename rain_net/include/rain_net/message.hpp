#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <cstring>
#include <ostream>
#include <type_traits>
#include <memory>
#include <limits>

namespace rain_net {
    namespace internal {
        inline constexpr std::size_t MAX_ITEM_SIZE = std::numeric_limits<std::uint16_t>::max();

        struct MsgHeader final {
            std::uint16_t id = 0;  // ID 0 is invalid
            std::uint16_t payload_size = 0;  // TODO memory layout
        };
    }

    class Connection;

    // Class representing a message, a blob of data
    // Messages can only contain trivially copyable types like numbers, C strings and POD structs
    class Message final {
    public:
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

            const std::size_t write_position = payload.size();

            payload.resize(payload.size() + sizeof(T));
            std::memcpy(payload.data() + write_position, &data, sizeof(T));

            header.payload_size = static_cast<std::uint16_t>(payload.size());

            return *this;
        }

        // Write data from message; reading must be done in reverse
        template<typename T>
        Message& operator>>(T& data) {
            static_assert(
                std::is_trivially_copyable_v<T>,
                "Type must be trivial, like a fundamental data type or a plain-old-data type"
            );
            static_assert(sizeof(T) <= internal::MAX_ITEM_SIZE);

            const std::size_t read_position = payload.size() - sizeof(T);

            std::memcpy(&data, payload.data() + read_position, sizeof(T));
            payload.resize(read_position);

            // Don't reset header.payload_size

            return *this;
        }
    private:
        internal::MsgHeader header;
        std::vector<unsigned char> payload;

        friend std::ostream& operator<<(std::ostream& stream, const Message& message);

        friend Message message(std::uint16_t id, std::size_t size_reserved);

        friend class Connection;
    };

    // Create a new empty message with specified ID and pre-allocated size
    Message message(std::uint16_t id, std::size_t size_reserved);

    // Print message
    std::ostream& operator<<(std::ostream& stream, const Message& message);

    // Convert enum type to ID type
    template<typename E>
    constexpr std::uint16_t id(E enum_id) {
        static_assert(std::is_enum_v<E>, "Type must be an enumeration");
        static_assert(sizeof(E) <= sizeof(std::uint16_t), "Enumeration type must be at most 2 bytes");

        return static_cast<std::uint16_t>(enum_id);
    }

    namespace internal {
        struct OwnedMsg final {
            Message message;
            std::shared_ptr<Connection> remote;
        };
    }
}
