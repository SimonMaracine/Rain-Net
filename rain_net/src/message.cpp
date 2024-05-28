#include "rain_net/message.hpp"

#include <ostream>

namespace rain_net {
    std::ostream& operator<<(std::ostream& stream, const Message& message) {
        stream
            << "Message(ID: "
            << message.header.id
            << ", payload: "
            << message.header.payload_size
            << " B)";

        return stream;
    }
}
