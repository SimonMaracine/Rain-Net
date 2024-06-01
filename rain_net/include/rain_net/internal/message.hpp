#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <memory>
#include <limits>

namespace rain_net {
    class ClientConnection;
    class ServerConnection;
    class MessageReader;

    namespace internal {
        class Connection;

        inline constexpr std::size_t MAX_ITEM_SIZE {std::numeric_limits<std::uint16_t>::max()};

        struct MsgHeader final {
            std::uint16_t id {};
            std::uint16_t payload_size {};
        };

        static_assert(std::is_trivially_copyable_v<MsgHeader>);
    }

    // Class representing a message, a blob of data
    // Messages can only contain trivially copyable types like numbers, C strings and POD structs
    class Message final {
    public:
        explicit Message(std::uint16_t id);

        Message(const Message& other);
        Message& operator=(const Message& other);
        Message(Message&& other) noexcept;
        Message& operator=(Message&& other) noexcept;

        ~Message() noexcept;

        std::size_t size() const;
        std::uint16_t id() const;

        // Write data to the message
        template<typename T>
        Message& operator<<(const T& data) {
            static_assert(std::is_trivially_copyable_v<T>);
            static_assert(sizeof(T) <= internal::MAX_ITEM_SIZE);

            const std::size_t write_position {header.payload_size};

            resize(sizeof(T));
            std::memcpy(payload + write_position, &data, sizeof(T));

            return *this;
        }
    private:
        Message() = default;

        void resize(std::size_t additional_size);
        void allocate(std::size_t size);

        internal::MsgHeader header;
        unsigned char* payload {nullptr};

        friend class internal::Connection;
        friend class ClientConnection;
        friend class ServerConnection;
        friend class MessageReader;
    };

    class MessageReader final {
    public:
        // Read data from the message; must be done in reverse
        template<typename T>
        MessageReader& operator>>(T& data) {
            static_assert(std::is_trivially_copyable_v<T>);
            static_assert(sizeof(T) <= internal::MAX_ITEM_SIZE);

            const std::size_t read_position {pointer - sizeof(T)};

            std::memcpy(&data, msg->payload + read_position, sizeof(T));
            pointer = read_position;

            return *this;
        }

        MessageReader& operator()(const Message& message);
    private:
        std::size_t pointer {};
        const Message* msg {nullptr};
    };

    namespace internal {
        struct OwnedMsg final {
            Message message {0};
            std::shared_ptr<ClientConnection> remote;
        };
    }
}
