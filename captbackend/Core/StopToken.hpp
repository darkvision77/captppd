#ifndef _CAPTBACKEND_CORE_STOP_TOKEN_HPP_
#define _CAPTBACKEND_CORE_STOP_TOKEN_HPP_

#include "Config.hpp"

#if HAVE_STOP_TOKEN

#include <stop_token>
typedef std::stop_token StopToken;
typedef std::stop_source StopSource;

#else

#include <atomic>

class StopSource;
class StopToken {
private:
    const StopSource* src;
public:
    StopToken() noexcept : src(nullptr) {}
    explicit StopToken(const StopSource& src) : src(&src) {}

    [[nodiscard]] bool stop_requested() const noexcept;
};

class StopSource {
private:
    std::atomic<bool> stopReq;
public:
    StopSource() noexcept : stopReq(false) {}

    StopSource(const StopSource&) = delete;
    StopSource& operator=(const StopSource&) = delete;

    StopSource(StopSource&& other) noexcept : stopReq(other.stopReq.load()) {}

    StopSource& operator=(StopSource&& other) noexcept {
        this->stopReq.store(other.stopReq.load());
        return *this;
    }

    bool request_stop() noexcept {
        if (!this->stopReq.exchange(true)) {
            return true;
        }
        return false;
    }

    [[nodiscard]] bool stop_requested() const noexcept {
        return this->stopReq.load();
    }

    [[nodiscard]] StopToken get_token() const noexcept {
        return StopToken(*this);
    }
};

inline bool StopToken::stop_requested() const noexcept {
    if (this->src == nullptr) {
        return false;
    }
    return this->src->stop_requested();
}

#endif
#endif
