#include "Core/StatusMessage.hpp"
#include <gtest/gtest.h>
#include <string_view>
#include <utility>
#include <type_traits>

using namespace Capt::Protocol;

struct Status {
    std::underlying_type_t<BasicStatus> Basic = 0;
    std::underlying_type_t<AuxStatus> Aux = 0;
    std::underlying_type_t<ControllerStatus> Controller = 0;
    std::underlying_type_t<EngineReadyStatus> Engine = 0;

    constexpr ExtendedStatus Make() const noexcept {
        return ExtendedStatus{
            .Basic = static_cast<BasicStatus>(this->Basic),
            .Changed = 0,
            .Aux = static_cast<AuxStatus>(this->Aux),
            .Controller = static_cast<ControllerStatus>(this->Controller),
            .PaperAvailableBits = 0,
            .Engine = static_cast<EngineReadyStatus>(this->Engine),
            .Start = 0,
            .Printing = 0,
            .Shipped = 0,
            .Printed = 0,
        };
    }

    friend std::ostream& operator<<(std::ostream& stream, const Status& status) {
        stream << "{ .Basic = " << static_cast<int>(status.Basic);
        stream << ", .Aux = " << static_cast<int>(status.Aux);
        stream << ", .Controller = " << static_cast<int>(status.Controller);
        stream << ", .Engine = " << static_cast<int>(status.Engine) << " }";
        return stream;
    }
};

TEST(StatusMessageTest, Basic) {
    const std::pair<Status, std::string_view> cases[] = {
        { {}, MsgReady },

        { { BasicStatus::NOT_READY }, MsgNotReady },
        { { BasicStatus::CMD_BUSY }, MsgUnknownFatal },
        { { BasicStatus::ERROR_BIT }, MsgUnknownFatal },
        { { BasicStatus::IM_DATA_BUSY }, MsgReady },
        { { BasicStatus::OFFLINE }, MsgReady },
        { { BasicStatus::UNIT_FREE }, MsgReady },

        { { 0, AuxStatus::PRINTER_BUSY }, MsgReady },
        { { 0, AuxStatus::PAPER_DELIVERY }, MsgPrinting },
        { { 0, AuxStatus::SAFE_TIMER }, MsgPrinting },

        { { 0, 0, ControllerStatus::ENGINE_RESET_IN_PROGRESS }, MsgWaiting },
        { { 0, 0, ControllerStatus::INVALID_DATA }, MsgVideoError },
        { { 0, 0, ControllerStatus::MISSING_EOP }, MsgVideoError },
        { { 0, 0, ControllerStatus::UNDERRUN }, MsgVideoError },
        { { 0, 0, ControllerStatus::OVERRUN }, MsgVideoError },
        { { 0, 0, ControllerStatus::ENGINE_COMM_ERROR }, MsgReady },
        { { 0, 0, ControllerStatus::PRINT_REJECTED }, MsgReady },

        { { 0, 0, 0, EngineReadyStatus::DOOR_OPEN }, MsgDoorOpen },
        { { 0, 0, 0, EngineReadyStatus::NO_CARTRIDGE }, MsgNoCartridge },
        { { 0, 0, 0, EngineReadyStatus::WAITING }, MsgWaiting },
        { { 0, 0, 0, EngineReadyStatus::TEST_PRINTING }, MsgPrinting },
        { { 0, 0, 0, EngineReadyStatus::NO_PRINT_PAPER }, MsgNoPaper },
        { { 0, 0, 0, EngineReadyStatus::JAM }, MsgJam },
        { { 0, 0, 0, EngineReadyStatus::CLEANING }, MsgCleaning },
        { { 0, 0, 0, EngineReadyStatus::SERVICE_CALL }, MsgServiceCall },
        { { 0, 0, 0, EngineReadyStatus::MIS_PRINT }, MsgReady },
        { { 0, 0, 0, EngineReadyStatus::MIS_PRINT_2 }, MsgReady },
    };

    for (const auto& [status, message] : cases) {
        EXPECT_EQ(StatusMessage(status.Make()), message) << status;
    }
}

TEST(StatusMessageTest, FatalOverlap) {
    const std::pair<Status, std::string_view> cases[] = {
        { { BasicStatus::NOT_READY, 0, 0, EngineReadyStatus::SERVICE_CALL }, MsgServiceCall },
        { { BasicStatus::CMD_BUSY, 0, 0, EngineReadyStatus::SERVICE_CALL }, MsgServiceCall },
        { { BasicStatus::ERROR_BIT, 0, 0, EngineReadyStatus::SERVICE_CALL }, MsgServiceCall },
        { { BasicStatus::IM_DATA_BUSY, 0, 0, EngineReadyStatus::SERVICE_CALL }, MsgServiceCall },
        { { BasicStatus::OFFLINE, 0, 0, EngineReadyStatus::SERVICE_CALL }, MsgServiceCall },
        { { BasicStatus::UNIT_FREE, 0, 0, EngineReadyStatus::SERVICE_CALL }, MsgServiceCall },

        { { BasicStatus::NOT_READY, AuxStatus::PAPER_DELIVERY | AuxStatus::SAFE_TIMER }, MsgPrinting },
        { { BasicStatus::CMD_BUSY, AuxStatus::PAPER_DELIVERY | AuxStatus::SAFE_TIMER }, MsgUnknownFatal },
        { { BasicStatus::ERROR_BIT, AuxStatus::PAPER_DELIVERY | AuxStatus::SAFE_TIMER }, MsgUnknownFatal },
        { { BasicStatus::IM_DATA_BUSY, AuxStatus::PAPER_DELIVERY | AuxStatus::SAFE_TIMER }, MsgPrinting },
        { { BasicStatus::OFFLINE, AuxStatus::PAPER_DELIVERY | AuxStatus::SAFE_TIMER }, MsgPrinting },
        { { BasicStatus::UNIT_FREE, AuxStatus::PAPER_DELIVERY | AuxStatus::SAFE_TIMER }, MsgPrinting },

        { { BasicStatus::NOT_READY, AuxStatus::PAPER_DELIVERY | AuxStatus::SAFE_TIMER, 0, EngineReadyStatus::SERVICE_CALL }, MsgServiceCall },
        { { BasicStatus::CMD_BUSY, AuxStatus::PAPER_DELIVERY | AuxStatus::SAFE_TIMER, 0, EngineReadyStatus::SERVICE_CALL }, MsgServiceCall },
        { { BasicStatus::ERROR_BIT, AuxStatus::PAPER_DELIVERY | AuxStatus::SAFE_TIMER, 0, EngineReadyStatus::SERVICE_CALL }, MsgServiceCall },
        { { BasicStatus::IM_DATA_BUSY, AuxStatus::PAPER_DELIVERY | AuxStatus::SAFE_TIMER, 0, EngineReadyStatus::SERVICE_CALL }, MsgServiceCall },
        { { BasicStatus::OFFLINE, AuxStatus::PAPER_DELIVERY | AuxStatus::SAFE_TIMER, 0, EngineReadyStatus::SERVICE_CALL }, MsgServiceCall },
        { { BasicStatus::UNIT_FREE, AuxStatus::PAPER_DELIVERY | AuxStatus::SAFE_TIMER, 0, EngineReadyStatus::SERVICE_CALL }, MsgServiceCall },

        { { BasicStatus::NOT_READY, AuxStatus::PAPER_DELIVERY | AuxStatus::SAFE_TIMER, 0, EngineReadyStatus::SERVICE_CALL }, MsgServiceCall },
        { { BasicStatus::CMD_BUSY, AuxStatus::PAPER_DELIVERY | AuxStatus::SAFE_TIMER, 0, EngineReadyStatus::SERVICE_CALL }, MsgServiceCall },
        { { BasicStatus::ERROR_BIT, AuxStatus::PAPER_DELIVERY | AuxStatus::SAFE_TIMER, 0, EngineReadyStatus::SERVICE_CALL }, MsgServiceCall },
        { { BasicStatus::IM_DATA_BUSY, AuxStatus::PAPER_DELIVERY | AuxStatus::SAFE_TIMER, 0, EngineReadyStatus::SERVICE_CALL }, MsgServiceCall },
        { { BasicStatus::OFFLINE, AuxStatus::PAPER_DELIVERY | AuxStatus::SAFE_TIMER, 0, EngineReadyStatus::SERVICE_CALL }, MsgServiceCall },
        { { BasicStatus::UNIT_FREE, AuxStatus::PAPER_DELIVERY | AuxStatus::SAFE_TIMER, 0, EngineReadyStatus::SERVICE_CALL }, MsgServiceCall },

        { { BasicStatus::NOT_READY, 0, ControllerStatus::ENGINE_RESET_IN_PROGRESS }, MsgWaiting },
        { { BasicStatus::CMD_BUSY, 0, ControllerStatus::ENGINE_RESET_IN_PROGRESS }, MsgUnknownFatal },
        { { BasicStatus::ERROR_BIT, 0, ControllerStatus::ENGINE_RESET_IN_PROGRESS }, MsgUnknownFatal },
        { { BasicStatus::IM_DATA_BUSY, 0, ControllerStatus::ENGINE_RESET_IN_PROGRESS }, MsgWaiting },
        { { BasicStatus::OFFLINE, 0, ControllerStatus::ENGINE_RESET_IN_PROGRESS }, MsgWaiting },
        { { BasicStatus::UNIT_FREE, 0, ControllerStatus::ENGINE_RESET_IN_PROGRESS }, MsgWaiting },

        { { BasicStatus::NOT_READY, 0, ControllerStatus::ENGINE_RESET_IN_PROGRESS, EngineReadyStatus::SERVICE_CALL }, MsgServiceCall },
        { { BasicStatus::CMD_BUSY, 0, ControllerStatus::ENGINE_RESET_IN_PROGRESS, EngineReadyStatus::SERVICE_CALL }, MsgServiceCall },
        { { BasicStatus::ERROR_BIT, 0, ControllerStatus::ENGINE_RESET_IN_PROGRESS, EngineReadyStatus::SERVICE_CALL }, MsgServiceCall },
        { { BasicStatus::IM_DATA_BUSY, 0, ControllerStatus::ENGINE_RESET_IN_PROGRESS, EngineReadyStatus::SERVICE_CALL }, MsgServiceCall },
        { { BasicStatus::OFFLINE, 0, ControllerStatus::ENGINE_RESET_IN_PROGRESS, EngineReadyStatus::SERVICE_CALL }, MsgServiceCall },
        { { BasicStatus::UNIT_FREE, 0, ControllerStatus::ENGINE_RESET_IN_PROGRESS, EngineReadyStatus::SERVICE_CALL }, MsgServiceCall },
    };

    for (const auto& [status, message] : cases) {
        EXPECT_EQ(StatusMessage(status.Make()), message) << status;
    }
}
