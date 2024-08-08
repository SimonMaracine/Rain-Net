#include "rain_net/internal/message.hpp"

#include <utility>

namespace rain_net {
    Message::Message(std::uint16_t id) noexcept {
        header.id = id;
    }

    Message::Message(internal::MsgHeader header, std::unique_ptr<unsigned char[]>&& payload) noexcept
        : header(header), payload(std::move(payload)) {}

    Message::Message(const Message& other) {
        if (other.payload != nullptr) {
            payload = std::make_unique<unsigned char[]>(other.header.payload_size);
            std::memcpy(payload.get(), other.payload.get(), other.header.payload_size);
        }

        header = other.header;
    }

    Message& Message::operator=(const Message& other) {
        if (other.payload != nullptr) {
            payload = std::make_unique<unsigned char[]>(other.header.payload_size);
            std::memcpy(payload.get(), other.payload.get(), other.header.payload_size);
        }

        header = other.header;

        return *this;
    }

    std::size_t Message::size() const noexcept {
        return sizeof(internal::MsgHeader) + header.payload_size;
    }

    std::uint16_t Message::id() const noexcept {
        return header.id;
    }

    Message& Message::write(const void* data, std::size_t size) {
        const std::size_t write_position {header.payload_size};

        resize(size);
        std::memcpy(payload.get() + write_position, data, size);

        return *this;
    }

    void Message::resize(std::size_t additional_size) {
        const std::size_t old_payload_size {header.payload_size};
        const auto old_payload {std::move(payload)};
        const std::size_t new_payload_size {header.payload_size + additional_size};

        payload = std::make_unique<unsigned char[]>(new_payload_size);

        if (old_payload != nullptr) {
            std::memcpy(payload.get(), old_payload.get(), old_payload_size);
        }

        header.payload_size = static_cast<std::uint16_t>(new_payload_size);
    }

    MessageReader& MessageReader::read(void* data, std::size_t size) noexcept {
        const std::size_t read_position {pointer - size};

        std::memcpy(data, msg->payload.get() + read_position, size);
        pointer = read_position;

        return *this;
    }

    MessageReader& MessageReader::operator()(const Message& message) noexcept {
        msg = &message;
        pointer = message.header.payload_size;

        return *this;
    }
}
