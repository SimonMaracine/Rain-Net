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
    // Messages can only contain trivially copyable types
    class Message final {
    public:
        explicit Message(std::uint16_t id) noexcept;

        Message(const Message& other);
        Message& operator=(const Message& other);
        Message(Message&& other) noexcept;
        Message& operator=(Message&& other) noexcept;

        ~Message() noexcept;

        // Get the size of the message, including header abd payload
        std::size_t size() const noexcept;

        // Get the message ID
        std::uint16_t id() const noexcept;

        // Write data to the message
        template<typename T>
        Message& operator<<(const T& data) {
            static_assert(std::is_trivially_copyable_v<T>);
            static_assert(sizeof(T) <= internal::MAX_ITEM_SIZE);

            return write(&data, sizeof(T));
        }

        // Write raw data to the message
        Message& write(const void* data, std::size_t size);
    private:
        Message() noexcept = default;

        void resize(std::size_t additional_size);
        void allocate(std::size_t size);  // Used externally

        internal::MsgHeader header;
        unsigned char* payload {nullptr};

        friend class internal::Connection;
        friend class ClientConnection;
        friend class ServerConnection;
        friend class MessageReader;
    };

    // Class used for reading messages
    class MessageReader final {
    public:
        // Read data from the message; must be done in reverse order
        template<typename T>
        MessageReader& operator>>(T& data) noexcept {
            static_assert(std::is_trivially_copyable_v<T>);
            static_assert(sizeof(T) <= internal::MAX_ITEM_SIZE);

            return read(&data, sizeof(T));
        }

        // Read raw data from the message; must be done in reverse order
        MessageReader& read(void* data, std::size_t size) noexcept;

        // Start reading the contents of a message
        MessageReader& operator()(const Message& message) noexcept;
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
