#include "rain_net/internal/message.hpp"

#include <utility>

namespace rain_net {
    Message::Message(std::uint16_t id) noexcept {
        header.id = id;
    }

    Message::Message(const Message& other) {
        if (other.payload != nullptr) {
            payload = new unsigned char[other.header.payload_size];
            std::memcpy(payload, other.payload, other.header.payload_size);
        }

        header = other.header;
    }

    Message& Message::operator=(const Message& other) {
        delete[] payload;

        if (other.payload != nullptr) {
            payload = new unsigned char[other.header.payload_size];
            std::memcpy(payload, other.payload, other.header.payload_size);
        }

        header = other.header;

        return *this;
    }

    Message::Message(Message&& other) noexcept
        : header(other.header), payload(std::exchange(other.payload, nullptr)) {}

    Message& Message::operator=(Message&& other) noexcept {
        delete[] payload;

        payload = std::exchange(other.payload, nullptr);
        header = other.header;

        return *this;
    }

    Message::~Message() noexcept {
        delete[] payload;
    }

    std::size_t Message::size() const noexcept {
        return sizeof(internal::MsgHeader) + header.payload_size;
    }

    std::uint16_t Message::id() const noexcept {
        return header.id;
    }

    void Message::resize(std::size_t additional_size) {
        const std::size_t old_payload_size {header.payload_size};
        unsigned char* old_payload {payload};
        const std::size_t new_payload_size {header.payload_size + additional_size};

        payload = new unsigned char[new_payload_size];

        if (old_payload != nullptr) {
            std::memcpy(payload, old_payload, old_payload_size);
        }

        header.payload_size = static_cast<std::uint16_t>(new_payload_size);

        delete[] old_payload;
    }

    void Message::allocate(std::size_t size) {
        payload = new unsigned char[size];
    }

    MessageReader& MessageReader::operator()(const Message& message) noexcept {
        msg = &message;
        pointer = message.header.payload_size;

        return *this;
    }
}
