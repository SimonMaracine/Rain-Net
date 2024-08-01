#pragma once

#include <atomic>
#include <string>
#include <string_view>
#include <utility>

namespace rain_net {
    namespace internal {
        class Errorable {
        public:
            // Check if there was an error
            bool fail() const noexcept {
                return error.load();
            }

            // Retrieve the error
            std::string_view fail_reason() const noexcept {
                return error_message;
            }
        protected:
            Errorable() noexcept = default;
            ~Errorable() noexcept = default;

            Errorable(const Errorable&) = delete;
            Errorable& operator=(const Errorable&) = delete;
            Errorable(Errorable&&) = delete;
            Errorable& operator=(Errorable&&) = delete;

            void set_error(const std::string& message) {
                set_error(std::string(message));
            }

            void set_error(std::string&& message) noexcept {
                error_message = std::move(message);
                error.store(true);
            }

            void clear_error() noexcept {
                error_message.clear();
                error.store(false);
            }
        private:
            std::string error_message;
            std::atomic_bool error {false};
        };
    }
}
