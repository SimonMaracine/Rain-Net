#pragma once

#include <atomic>
#include <string>
#include <string_view>
#include <utility>

namespace rain_net {
    namespace internal {
        class Errorable {
        public:
            ~Errorable() = default;

            Errorable(const Errorable&) = delete;
            Errorable& operator=(const Errorable&) = delete;
            Errorable(Errorable&&) = delete;
            Errorable& operator=(Errorable&&) = delete;

            // Check if there was an error
            bool fail() const {
                return error.load();
            }

            // Retrieve the error
            std::string_view fail_reason() const {
                return error_message;
            }
        protected:
            Errorable() = default;

            void set_error(const std::string& message) {
                error_message = message;
                error.store(true);
            }

            void set_error(std::string&& message) {
                error_message = std::move(message);
                error.store(true);
            }

            void clear_error() {
                error_message.clear();
                error.store(false);
            }
        private:
            std::atomic_bool error {false};
            std::string error_message;
        };
    }
}
