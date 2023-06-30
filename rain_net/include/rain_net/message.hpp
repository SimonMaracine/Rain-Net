#pragma once

#include <cstdint>
#include <vector>
#include <cstring>
#include <iostream>
#include <type_traits>
#include <memory>
#include <limits>

namespace rain_net {
    namespace internal {
        inline constexpr size_t MAX_ITEM_SIZE = std::numeric_limits<uint16_t>::max();

        template<typename E>
        struct MsgHeader final {
            MsgHeader() {
                static_assert(std::is_enum_v<E>, "Type must be an enumeration");
            }

            E id {};
            uint16_t payload_size = 0;  // TODO memory layout
        };
    }

    template<typename E>
    class Connection;

    template<typename E>
    class Message final {
    public:
        size_t size() const {
            return sizeof(internal::MsgHeader<E>) + header.payload_size;
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
            static_assert(sizeof(T) <= internal::MAX_ITEM_SIZE);

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
            static_assert(sizeof(T) <= internal::MAX_ITEM_SIZE);

            const size_t read_position = payload.size() - sizeof(T);

            std::memcpy(&data, payload.data() + read_position, sizeof(T));
            payload.resize(read_position);

            // Don't reset header.payload_size

            return *this;
        }
    private:
        internal::MsgHeader<E> header;
        std::vector<uint8_t> payload;

        template<typename F>
        friend std::ostream& operator<<(std::ostream& stream, const Message<F>& message);

        template<typename F>
        friend Message<F> message(F id, size_t size_reserved);

        template<typename F>
        friend class Connection;
    };

    template<typename E>
    Message<E> message(E id, size_t size_reserved) {
        Message<E> msg;
        msg.header.id = id;
        msg.payload.reserve(size_reserved);

        return msg;
    }

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

    namespace internal {
        template<typename E>
        struct OwnedMsg final {
            Message<E> message;
            std::shared_ptr<Connection<E>> remote;
        };
    }
}
