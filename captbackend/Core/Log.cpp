#include "Log.hpp"
#include <cassert>
#include <iostream>

namespace Log {
    static std::ostream* LogStream = &std::clog;

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

    void SetLogStream(std::ostream& stream) noexcept {
        LogStream = &stream;
    }

    StreamTerminator Log(std::string_view level) {
        assert(LogStream != nullptr);
        return StreamTerminator(*LogStream) << level << ": ";
    }
}
