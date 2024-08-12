#include "rain_net/internal/message.hpp"

#include <utility>
#include <cstring>

namespace rain_net {
    namespace internal {
        BasicMessage clone_message(const Message& message) {
            internal::BasicMessage result;
            result.header = message.m_header;
            result.payload = std::make_unique<unsigned char[]>(message.m_header.payload_size);
            std::memcpy(result.payload.get(), message.m_payload.get(), message.m_header.payload_size);

            return result;
        }
    }

    Message::Message(std::uint16_t id) noexcept {
        m_header.id = id;
    }

    Message::Message(internal::MsgHeader header, std::unique_ptr<unsigned char[]>&& payload) noexcept
        : m_header(header), m_payload(std::move(payload)) {}

    Message::Message(const Message& other) {
        if (other.m_payload != nullptr) {
            m_payload = std::make_unique<unsigned char[]>(other.m_header.payload_size);
            std::memcpy(m_payload.get(), other.m_payload.get(), other.m_header.payload_size);
        }

        m_header = other.m_header;
    }

    Message& Message::operator=(const Message& other) {
        if (other.m_payload != nullptr) {
            m_payload = std::make_unique<unsigned char[]>(other.m_header.payload_size);
            std::memcpy(m_payload.get(), other.m_payload.get(), other.m_header.payload_size);
        }

        m_header = other.m_header;

        return *this;
    }

    std::size_t Message::size() const noexcept {
        return sizeof(internal::MsgHeader) + m_header.payload_size;
    }

    std::uint16_t Message::id() const noexcept {
        return m_header.id;
    }

    Message& Message::write(const void* data, std::size_t size) {
        const std::size_t write_position {m_header.payload_size};

        resize(size);
        std::memcpy(m_payload.get() + write_position, data, size);

        return *this;
    }

    void Message::resize(std::size_t additional_size) {
        const std::size_t old_payload_size {m_header.payload_size};
        const auto old_payload {std::move(m_payload)};
        const std::size_t new_payload_size {m_header.payload_size + additional_size};

        m_payload = std::make_unique<unsigned char[]>(new_payload_size);

        if (old_payload != nullptr) {
            std::memcpy(m_payload.get(), old_payload.get(), old_payload_size);
        }

        m_header.payload_size = static_cast<std::uint16_t>(new_payload_size);
    }

    MessageReader& MessageReader::read(void* data, std::size_t size) noexcept {
        const std::size_t read_position {m_pointer - size};

        std::memcpy(data, m_message->m_payload.get() + read_position, size);
        m_pointer = read_position;

        return *this;
    }

    MessageReader& MessageReader::operator()(const Message& message) noexcept {
        m_message = &message;
        m_pointer = message.m_header.payload_size;

        return *this;
    }
}
