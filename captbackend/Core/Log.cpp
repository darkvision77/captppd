#include "Log.hpp"

namespace Log {
    StreamTerminator::StreamTerminator(std::ostream& stream) noexcept : stream(stream) {}

    StreamTerminator::StreamTerminator(StreamTerminator&& other) noexcept : stream(other.stream) {
        other.terminate = false;
    }

    StreamTerminator::~StreamTerminator() {
        if (this->terminate) {
            this->stream << std::endl;
        }
    }

    StreamTerminator Log(std::string_view level) {
        return StreamTerminator(std::cerr) << level << ": ";
    }
}
