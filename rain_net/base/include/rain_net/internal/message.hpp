#pragma once

#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <memory>
#include <limits>

namespace rain_net {
    class MessageReader;
    class Message;

    namespace internal {
        inline constexpr std::size_t MAX_ITEM_SIZE {std::numeric_limits<std::uint16_t>::max()};

        struct MsgHeader final {
            std::uint16_t id {};
            std::uint16_t payload_size {};
        };

        static_assert(std::is_trivially_copyable_v<MsgHeader>);

        struct BasicMessage final {
            MsgHeader header;
            std::unique_ptr<unsigned char[]> payload;
        };

        BasicMessage clone_message(const Message& message);
    }

    // Class representing a message, a blob of data
    // Messages can only contain data from trivially copyable types
    class Message final {
    public:
        Message() noexcept = default;
        explicit Message(std::uint16_t id) noexcept;
        Message(internal::MsgHeader header, std::unique_ptr<unsigned char[]>&& payload) noexcept;

        ~Message() noexcept = default;

        Message(const Message& other);
        Message& operator=(const Message& other);
        Message(Message&&) noexcept = default;
        Message& operator=(Message&&) noexcept = default;

        // Get the size of the message, including header and payload
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

        internal::MsgHeader m_header;
        std::unique_ptr<unsigned char[]> m_payload;

        friend class MessageReader;
        friend internal::BasicMessage internal::clone_message(const Message& message);
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
        std::size_t m_pointer {};
        const Message* m_message {nullptr};
    };
}
