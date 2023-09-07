#include <cstdint>
#include <cstddef>
#include <vector>
#include <cstring>
#include <iostream>
#include <type_traits>
#include <memory>
#include <limits>

#include "rain_net/message.hpp"

namespace rain_net {
    Message message(std::uint16_t id, std::size_t size_reserved) {
        Message msg;
        msg.header.id = id;
        msg.payload.reserve(size_reserved);

        return msg;
    }

    std::ostream& operator<<(std::ostream& stream, const Message& message) {
        stream
            << "Message { ID: "
            << message.header.id
            << ", payload: "
            << message.header.payload_size
            << " B }";

        return stream;
    }
}
