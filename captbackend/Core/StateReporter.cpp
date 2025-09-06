#include "StateReporter.hpp"
#include <algorithm>
#include <string_view>

using namespace Capt::Protocol;

StateReporter::StateReporter(std::ostream& stream) noexcept : stream(stream) {}

void StateReporter::Update(ExtendedStatus status) {
    bool serviceCall = (status.Engine & EngineReadyStatus::SERVICE_CALL) != 0;
    bool fatal = status.FatalError();
    if (serviceCall || fatal) {
        this->Clear();
        this->SetReason("other-error", serviceCall);
        this->SetReason("unknown-error", fatal && !serviceCall);
        return;
    }
    this->SetReason("other-error", false);
    this->SetReason("unknown-error", false);

    this->SetReason("media-empty-error", (status.Engine & EngineReadyStatus::NO_PRINT_PAPER) != 0);
    this->SetReason("media-needed-error", (status.Engine & EngineReadyStatus::NO_PRINT_PAPER) != 0);
    this->SetReason("media-jam-error", (status.Engine & EngineReadyStatus::JAM) != 0);

    this->SetReason("toner-empty-error", (status.Engine & EngineReadyStatus::NO_CARTRIDGE) != 0);
    this->SetReason("door-open-error", (status.Engine & EngineReadyStatus::DOOR_OPEN) != 0);

    bool waiting = (status.Engine & EngineReadyStatus::WAITING) != 0
        | (status.Controller & ControllerStatus::ENGINE_RESET_IN_PROGRESS) != 0;
    this->SetReason("resuming", waiting);
}

void StateReporter::SetReason(std::string_view reason, bool set) {
    bool contains = std::ranges::find(this->reasons.cbegin(), this->reasons.cend(), reason) != this->reasons.cend();
    if (set == contains) {
        return;
    }
    this->stream << "STATE: " << (set ? '+' : '-') << reason << std::endl;
    if (set) {
        this->reasons.insert(reason);
    } else {
        this->reasons.erase(reason);
    }
}

void StateReporter::Clear() {
    for (const std::string_view s : this->reasons) {
        this->stream << "STATE: -" << s << std::endl;
    }
    this->reasons.clear();
}

void StateReporter::Page(unsigned page) {
    this->stream << "PAGE: page-number " << page << std::endl;
}
