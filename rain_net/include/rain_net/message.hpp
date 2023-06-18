#pragma once

#include <cstdint>
#include <vector>
#include <cstring>
#include <iostream>
#include <type_traits>

namespace rain_net {
    template<typename E>
    struct MsgHeader {
        MsgHeader() {
            static_assert(std::is_enum_v<E>, "Type must be an enumeration");
            static_assert(sizeof(E) <= 8, "Enum must be at most 8 bytes in size");
        }

        E id {};
        uint32_t payload_size = 0;
    };

    template<typename E>
    struct Message {
        MsgHeader<E> header;
        std::vector<uint8_t> payload;

        size_t size() const {
            return sizeof(MsgHeader<E>) + payload.size();
        }

        template<typename T>
        Message& operator<<(const T& data) {  // TODO optimize
            static_assert(
                std::is_trivially_copyable_v<T>,
                "Type must be trivial, like a fundamental data type or a plain-old-data type"
            );

            payload.resize(payload.size() + sizeof(T));
            std::memcpy(payload.data() + payload.size() - sizeof(T), &data, sizeof(T));

            header.payload_size = payload.size();

            return *this;
        }

        template<typename T>
        Message& operator>>(T& data) {  // TODO optimize
            static_assert(
                std::is_trivially_copyable_v<T>,
                "Type must be trivial, like a fundamental data type or a plain-old-data type"
            );

            std::memcpy(&data, payload.data() + payload.size() - sizeof(T), sizeof(T));
            payload.resize(payload.size() - sizeof(T));

            // FIXME what about header.payload_size?

            return *this;
        }
    };

    template<typename E>
    std::ostream& operator<<(std::ostream& stream, const Message<E>& message) {
        stream
            << "Message { ID: "
            << static_cast<std::underlying_type_t<E>>(message.header.id)
            << ", payload: "
            << message.payload.size()
            << " B }";

        return stream;
    }
}
