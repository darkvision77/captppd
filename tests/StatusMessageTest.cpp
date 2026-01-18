#include "Core/StatusMessage.hpp"
#include <libcapt/Protocol/ExtendedStatus.hpp>
#include <gtest/gtest.h>
#include <string_view>
#include <utility>
#include <type_traits>

using namespace Capt;

struct Status {
    std::underlying_type_t<BasicStatus> Basic = 0;
    std::underlying_type_t<AuxStatus> Aux = 0;
    std::underlying_type_t<ControllerStatus> Controller = 0;
    std::underlying_type_t<EngineReadyStatus> Engine = 0;
    bool PaperBit = true;

    constexpr ExtendedStatus Make() const noexcept {
        return ExtendedStatus{
            .Basic = static_cast<BasicStatus>(this->Basic),
            .Changed = 0,
            .Aux = static_cast<AuxStatus>(this->Aux),
            .Controller = static_cast<ControllerStatus>(this->Controller),
            .PaperAvailableBits = static_cast<uint8_t>(this->PaperBit ? 0x80 : 0),
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
        { { .PaperBit = false }, MsgNoPaper },

        { { .Basic = BasicStatus::NOT_READY }, MsgNotReady },
        { { .Basic = BasicStatus::NOT_READY, .PaperBit = false }, MsgNoPaper },
        { { .Basic = BasicStatus::CMD_BUSY }, MsgUnknownFatal },
        { { .Basic = BasicStatus::ERROR_BIT }, MsgUnknownFatal },
        { { .Basic = BasicStatus::IM_DATA_BUSY }, MsgReady },
        { { .Basic = BasicStatus::OFFLINE }, MsgReady },
        { { .Basic = BasicStatus::UNIT_FREE }, MsgReady },
        { { .Basic = BasicStatus::IM_DATA_BUSY, .PaperBit = false }, MsgNoPaper },
        { { .Basic = BasicStatus::OFFLINE, .PaperBit = false }, MsgNoPaper },
        { { .Basic = BasicStatus::UNIT_FREE, .PaperBit = false }, MsgNoPaper },

        { { .Aux = AuxStatus::PRINTER_BUSY }, MsgReady },
        { { .Aux = AuxStatus::PAPER_DELIVERY }, MsgPrinting },
        { { .Aux = AuxStatus::SAFE_TIMER }, MsgPrinting },

        { { .Controller = ControllerStatus::ENGINE_RESET_IN_PROGRESS }, MsgWaiting },
        { { .Controller = ControllerStatus::INVALID_DATA }, MsgVideoError },
        { { .Controller = ControllerStatus::MISSING_EOP }, MsgVideoError },
        { { .Controller = ControllerStatus::UNDERRUN }, MsgVideoError },
        { { .Controller = ControllerStatus::OVERRUN }, MsgVideoError },
        { { .Controller = ControllerStatus::ENGINE_COMM_ERROR }, MsgReady },
        { { .Controller = ControllerStatus::PRINT_REJECTED }, MsgReady },
        { { .Controller = ControllerStatus::ENGINE_COMM_ERROR, .PaperBit = false }, MsgNoPaper },
        { { .Controller = ControllerStatus::PRINT_REJECTED, .PaperBit = false }, MsgNoPaper },

        { { .Engine = EngineReadyStatus::DOOR_OPEN }, MsgDoorOpen },
        { { .Engine = EngineReadyStatus::NO_CARTRIDGE }, MsgNoCartridge },
        { { .Engine = EngineReadyStatus::WAITING }, MsgWaiting },
        { { .Engine = EngineReadyStatus::TEST_PRINTING }, MsgPrinting },
        { { .Engine = EngineReadyStatus::NO_PRINT_PAPER }, MsgNoPaper },
        { { .Engine = EngineReadyStatus::JAM }, MsgJam },
        { { .Engine = EngineReadyStatus::CLEANING }, MsgCleaning },
        { { .Engine = EngineReadyStatus::SERVICE_CALL }, MsgServiceCall },
        { { .Engine = EngineReadyStatus::MIS_PRINT }, MsgReady },
        { { .Engine = EngineReadyStatus::MIS_PRINT_2 }, MsgReady },
        { { .Engine = EngineReadyStatus::MIS_PRINT, .PaperBit = false }, MsgNoPaper },
        { { .Engine = EngineReadyStatus::MIS_PRINT_2, .PaperBit = false }, MsgNoPaper },
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
