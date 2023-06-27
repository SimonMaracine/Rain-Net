#pragma once

#include <cstdint>
#include <vector>
#include <cstring>
#include <iostream>
#include <type_traits>
#include <memory>

namespace rain_net {
    template<typename E>
    class Connection;

    template<typename F>
    class ClientConnection;

    template<typename E>
    struct MsgHeader final {  // TODO hide this
        MsgHeader() {
            static_assert(std::is_enum_v<E>, "Type must be an enumeration");
        }

        E id {};
        uint32_t payload_size = 0;
    };

    template<typename E>
    class Message final {
    public:
        size_t size() const {
            return sizeof(MsgHeader<E>) + header.payload_size;
        }

        E id() const {
            return header.id;
        }

        template<typename T>
        Message& operator<<(const T& data) {
            static_assert(
                std::is_trivially_copyable_v<T>,
                "Type must be trivial, like a fundamental data type or a plain-old-data type"
            );

            const size_t write_position = payload.size();

            payload.resize(payload.size() + sizeof(T));
            std::memcpy(payload.data() + write_position, &data, sizeof(T));

            header.payload_size = payload.size();

            return *this;
        }

        template<typename T>
        Message& operator>>(T& data) {
            static_assert(
                std::is_trivially_copyable_v<T>,
                "Type must be trivial, like a fundamental data type or a plain-old-data type"
            );

            const size_t read_position = payload.size() - sizeof(T);

            std::memcpy(&data, payload.data() + read_position, sizeof(T));
            payload.resize(read_position);

            // Don't reset header.payload_size

            return *this;
        }
    private:
        MsgHeader<E> header;
        std::vector<uint8_t> payload;

        template<typename F>
        friend std::ostream& operator<<(std::ostream& stream, const Message<F>& message);

        template<typename F>
        friend Message<F> new_message(F id, size_t size_reserved);

        template<typename F>
        friend class Connection;
    };

    template<typename E>
    struct OwnedMessage final {
        Message<E> msg;
        std::shared_ptr<Connection<E>> remote;
    };

    template<typename E>
    std::ostream& operator<<(std::ostream& stream, const Message<E>& message) {
        stream
            << "Message { ID: "
            << static_cast<std::underlying_type_t<E>>(message.header.id)
            << ", payload: "
            << message.header.payload_size
            << " B }";

        return stream;
    }

    template<typename E>
    std::ostream& operator<<(std::ostream& stream, const OwnedMessage<E>& message) {
        stream << message;

        return stream;
    }

    template<typename E>
    Message<E> new_message(E id, size_t size_reserved) {
        Message<E> message;
        message.header.id = id;
        message.payload.reserve(size_reserved);

        return message;
    }
}
