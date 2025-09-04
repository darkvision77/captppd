#ifndef _CAPTBACKEND_CORE_STATUS_MESSAGE_HPP_
#define _CAPTBACKEND_CORE_STATUS_MESSAGE_HPP_

#include <string_view>
#include <libcapt/Protocol/ExtendedStatus.hpp>

constexpr std::string_view StatusMessage(const Capt::Protocol::ExtendedStatus& status) noexcept {
    using namespace Capt::Protocol;
    if ((status.Engine & EngineReadyStatus::SERVICE_CALL) != 0) {
        return "Engine fault";
    }
    if (status.FatalError()) {
        return "Unknown fatal error";
    }
    bool waiting = (status.Engine & EngineReadyStatus::WAITING) != 0
        | (status.Engine & EngineReadyStatus::TEST_PRINTING) != 0
        | (status.Controller & ControllerStatus::ENGINE_RESET_IN_PROGRESS) != 0;
    if (waiting) {
        return "Waiting";
    }
    if ((status.Engine & EngineReadyStatus::DOOR_OPEN) != 0) {
        return "Door open";
    }
    if ((status.Engine & EngineReadyStatus::NO_CARTRIDGE) != 0) {
        return "No cartridge";
    }
    if ((status.Engine & EngineReadyStatus::JAM) != 0) {
        return "Paper jam";
    }
    if ((status.Engine & EngineReadyStatus::NO_PRINT_PAPER) != 0) {
        return "Out of paper";
    }
    return "Ready";
}

#endif
