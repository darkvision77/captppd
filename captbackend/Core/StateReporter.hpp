#pragma once
#include <ostream>
#include <libcapt/Protocol/ExtendedStatus.hpp>
#include <string_view>
#include <unordered_set>

class StateReporter {
private:
    std::ostream& stream;
    std::unordered_set<std::string_view> reasons;
public:
    // stream MUST be noexcept
    explicit StateReporter(std::ostream& stream) noexcept;
    ~StateReporter() noexcept;

    void Update(Capt::ExtendedStatus status);
    void SetReason(std::string_view reason, bool set);
    void Clear() noexcept;

    void Page(unsigned page) noexcept;
};
