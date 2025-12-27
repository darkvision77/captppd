#pragma once
#include <string_view>
#include <libcapt/Protocol/ExtendedStatus.hpp>

inline constexpr std::string_view MsgReady = "Ready";
inline constexpr std::string_view MsgNotReady = "Not ready";
inline constexpr std::string_view MsgUnknownFatal = "Unknown fatal error";
inline constexpr std::string_view MsgVideoError = "Video data error";
inline constexpr std::string_view MsgDoorOpen = "Door open";
inline constexpr std::string_view MsgJam = "Paper jam";
inline constexpr std::string_view MsgNoCartridge = "No cartridge";
inline constexpr std::string_view MsgNoPaper = "Out of paper";
inline constexpr std::string_view MsgWaiting = "Waiting";
inline constexpr std::string_view MsgServiceCall = "Service call";
inline constexpr std::string_view MsgPrinting = "Printing";
inline constexpr std::string_view MsgCleaning = "Cleaning";

[[nodiscard]] constexpr std::string_view StatusMessage(Capt::ExtendedStatus status) noexcept {
    using namespace Capt;
    if ((status.Engine & EngineReadyStatus::SERVICE_CALL) != 0) {
        return MsgServiceCall;
    }
    if (status.FatalError()) {
        return MsgUnknownFatal;
    }
    if (status.VideoDataError()) {
        return MsgVideoError;
    }
    bool waiting = (status.Engine & EngineReadyStatus::WAITING) != 0
        || (status.Controller & ControllerStatus::ENGINE_RESET_IN_PROGRESS) != 0;
    if (waiting) {
        return MsgWaiting;
    }
    if ((status.Engine & EngineReadyStatus::DOOR_OPEN) != 0) {
        return MsgDoorOpen;
    }
    if ((status.Engine & EngineReadyStatus::JAM) != 0) {
        return MsgJam;
    }
    if ((status.Engine & EngineReadyStatus::NO_CARTRIDGE) != 0) {
        return MsgNoCartridge;
    }
    if ((status.Engine & EngineReadyStatus::NO_PRINT_PAPER) != 0) {
        return MsgNoPaper;
    }
    if ((status.Engine & EngineReadyStatus::CLEANING) != 0) {
        return MsgCleaning;
    }
    if (status.IsPrinting() || ((status.Engine & EngineReadyStatus::TEST_PRINTING) != 0)) {
        return MsgPrinting;
    }
    if (!status.Ready()) {
        return MsgNotReady;
    }
    return MsgReady;
}
