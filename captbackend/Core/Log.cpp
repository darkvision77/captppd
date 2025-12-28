#include "Log.hpp"

namespace Log {
    StreamTerminator::StreamTerminator(std::ostream& stream) noexcept
        : stream(stream) {
        this->state.copyfmt(stream);
    }

    StreamTerminator::StreamTerminator(StreamTerminator&& other) noexcept
        : stream(other.stream) {
        this->state.copyfmt(other.state);
        other.terminate = false;
    }

    StreamTerminator::~StreamTerminator() {
        if (this->terminate) {
            this->stream << std::endl;
            this->stream.copyfmt(this->state);
        }
    }

    StreamTerminator Log(std::string_view level) {
        return StreamTerminator(std::cerr) << level << ": ";
    }
}
