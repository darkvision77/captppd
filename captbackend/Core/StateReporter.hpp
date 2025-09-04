#ifndef _CAPTBACKEND_CORE_STATE_REPORTER_HPP_
#define _CAPTBACKEND_CORE_STATE_REPORTER_HPP_

#include <ostream>
#include <libcapt/Protocol/ExtendedStatus.hpp>
#include <string_view>
#include <unordered_set>

class StateReporter {
private:
    std::ostream& stream;
    std::unordered_set<std::string_view> reasons;
public:
    explicit StateReporter(std::ostream& stream);

    inline ~StateReporter() {
        this->Clear();
    }

    void Update(const Capt::Protocol::ExtendedStatus& status);
    void SetReason(std::string_view reason, bool set);
    void Clear();

    void Page(unsigned page);
};

#endif
