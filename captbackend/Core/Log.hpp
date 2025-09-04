#ifndef _CAPTBACKEND_CORE_LOG_HPP_
#define _CAPTBACKEND_CORE_LOG_HPP_

#include <iostream>
#include <string_view>

namespace Log {
    class StreamTerminator {
    private:
        std::ostream& stream;
        bool terminate = true;
    public:
        explicit StreamTerminator(std::ostream& stream) noexcept;
        StreamTerminator(StreamTerminator&& other) noexcept;
        StreamTerminator(const StreamTerminator&) = delete;

        ~StreamTerminator();

        template<typename T>
        friend StreamTerminator operator<<(StreamTerminator&& stream, const T& val) {
            stream.stream << val;
            return std::move(stream);
        }

        template<typename T>
        friend StreamTerminator operator<<(StreamTerminator& stream, const T& val) {
            return StreamTerminator(stream.stream) << val;
        }
    };

    StreamTerminator Log(std::string_view level);

    inline StreamTerminator Debug() { return Log("DEBUG"); }
    inline StreamTerminator Info() { return Log("INFO"); }
    inline StreamTerminator Warning() { return Log("WARN"); }
    inline StreamTerminator Error() { return Log("ERROR"); }
    inline StreamTerminator Critical() { return Log("CRIT"); }
}

#endif
