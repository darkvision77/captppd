#pragma once
#include <string_view>
#include <libcapt/Protocol/ExtendedStatus.hpp>

constexpr inline std::string_view MsgReady = "Ready";
constexpr inline std::string_view MsgNotReady = "Not ready";
constexpr inline std::string_view MsgUnknownFatal = "Unknown fatal error";
constexpr inline std::string_view MsgVideoError = "Video data error";
constexpr inline std::string_view MsgDoorOpen = "Door open";
constexpr inline std::string_view MsgJam = "Paper jam";
constexpr inline std::string_view MsgNoCartridge = "No cartridge";
constexpr inline std::string_view MsgNoPaper = "Out of paper";
constexpr inline std::string_view MsgWaiting = "Waiting";
constexpr inline std::string_view MsgServiceCall = "Service call";
constexpr inline std::string_view MsgPrinting = "Printing";
constexpr inline std::string_view MsgCleaning = "Cleaning";

[[nodiscard]] constexpr std::string_view StatusMessage(Capt::ExtendedStatus status) noexcept {
    using namespace Capt;
    if (status.ServiceCall() != 0) {
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
    if ((status.Engine & EngineReadyStatus::CLEANING) != 0) {
        return MsgCleaning;
    }
    if (status.IsPrinting() || ((status.Engine & EngineReadyStatus::TEST_PRINTING) != 0)) {
        return MsgPrinting;
    }
    if ((status.Engine & EngineReadyStatus::NO_PRINT_PAPER) != 0 || status.PaperAvailableBits == 0) {
        return MsgNoPaper;
    }
    if (!status.Ready()) {
        return MsgNotReady;
    }
    return MsgReady;
}
