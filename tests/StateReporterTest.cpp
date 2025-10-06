#include "Core/StateReporter.hpp"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <string_view>
#include <unordered_set>
#include <type_traits>
#include <ranges>
#include <sstream>
#include <string_view>

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

class StateParser {
public:
    std::unordered_set<std::string> Reasons;
    unsigned Page = 0;

    void Parse(std::string_view s) {
        for (const auto line : (s | std::views::split('\n'))) {
            #if defined(__GNUC__) && __GNUC__ < 12
                auto c = line | std::views::common;
                std::string str(c.begin(), c.end());
            #else
                std::string_view str(line);
            #endif
            if (str.starts_with("STATE: ")) {
                std::string reason(str.substr(8));
                char act = str.at(7);
                ASSERT_TRUE(act == '+' || act == '-') << act;
                if (act == '+') {
                    EXPECT_FALSE(this->Reasons.contains(reason));
                    this->Reasons.insert(reason);
                } else {
                    EXPECT_TRUE(this->Reasons.contains(reason));
                    this->Reasons.erase(reason);
                }
            } else if (str.starts_with("PAGE: page-number ")) {
                int page = std::stoi(std::string(str.substr(18)));
                ASSERT_GE(page, 0);
                this->Page = page;
            } else if (str.size() != 0) {
                FAIL() << "failed to parse line: " << str;
            }
        }
    }

    void Reset() noexcept {
        this->Reasons.clear();
        this->Page = 0;
    }
};

class StateReporterTest : public testing::Test {
public:
    std::ostringstream Stream;
    StateParser Parser;
    StateReporter Reporter;

    StateReporterTest() : Reporter(Stream) {}

    void Flush() {
        this->Stream.str("");
    }

    void Parse() {
        this->Parser.Parse(this->Stream.str());
        this->Flush();
    }

    void Update(Status s) {
        Reporter.Update(s.Make());
        this->Parse();
    }

    void ResetUpdate(Status s) {
        this->Parser.Reset();
        this->Reporter.Clear();
        this->Flush();
        this->Update(s);
    }
};

TEST_F(StateReporterTest, PageReport) {
    Reporter.Page(0);
    Parse();
    EXPECT_EQ(Parser.Reasons.size(), 0);
    EXPECT_EQ(Parser.Page, 0);

    Reporter.Page(123);
    Parse();
    EXPECT_EQ(Parser.Reasons.size(), 0);
    EXPECT_EQ(Parser.Page, 123);

    Reporter.Clear();
    Parse();
    EXPECT_EQ(Parser.Reasons.size(), 0);
    EXPECT_EQ(Parser.Page, 123);
}

TEST_F(StateReporterTest, Clear) {
    ResetUpdate({.Engine = EngineReadyStatus::NO_PRINT_PAPER});
    EXPECT_THAT(Parser.Reasons, testing::UnorderedElementsAre("media-needed-error", "media-empty-error"));

    Reporter.Clear();
    Parse();
    EXPECT_EQ(Parser.Reasons.size(), 0);
}

TEST_F(StateReporterTest, Basic) {
    ResetUpdate({});
    EXPECT_EQ(Parser.Reasons.size(), 0);

    ResetUpdate({BasicStatus::CMD_BUSY});
    EXPECT_THAT(Parser.Reasons, testing::UnorderedElementsAre("unknown-error"));

    ResetUpdate({BasicStatus::ERROR_BIT});
    EXPECT_THAT(Parser.Reasons, testing::UnorderedElementsAre("unknown-error"));

    ResetUpdate({.Controller = ControllerStatus::ENGINE_RESET_IN_PROGRESS});
    EXPECT_THAT(Parser.Reasons, testing::UnorderedElementsAre("resuming"));

    ResetUpdate({.Engine = EngineReadyStatus::WAITING});
    EXPECT_THAT(Parser.Reasons, testing::UnorderedElementsAre("resuming"));

    ResetUpdate({.Engine = EngineReadyStatus::DOOR_OPEN});
    EXPECT_THAT(Parser.Reasons, testing::UnorderedElementsAre("door-open-error"));

    ResetUpdate({.Engine = EngineReadyStatus::NO_CARTRIDGE});
    EXPECT_THAT(Parser.Reasons, testing::UnorderedElementsAre("toner-empty-error"));

    ResetUpdate({.Engine = EngineReadyStatus::NO_PRINT_PAPER});
    EXPECT_THAT(Parser.Reasons, testing::UnorderedElementsAre("media-needed-error", "media-empty-error"));

    ResetUpdate({.Engine = EngineReadyStatus::JAM});
    EXPECT_THAT(Parser.Reasons, testing::UnorderedElementsAre("media-jam-error"));

    ResetUpdate({.Engine = EngineReadyStatus::SERVICE_CALL});
    EXPECT_THAT(Parser.Reasons, testing::UnorderedElementsAre("other-error"));
}

TEST_F(StateReporterTest, Multiple) {
    ResetUpdate({.Engine = EngineReadyStatus::NO_PRINT_PAPER | EngineReadyStatus::JAM});
    EXPECT_THAT(Parser.Reasons, testing::UnorderedElementsAre("media-needed-error", "media-empty-error", "media-jam-error"));

    Update({});
    EXPECT_EQ(Parser.Reasons.size(), 0);
}

TEST_F(StateReporterTest, FatalOverlap) {
    ResetUpdate({.Basic = BasicStatus::CMD_BUSY, .Engine = EngineReadyStatus::NO_PRINT_PAPER});
    EXPECT_THAT(Parser.Reasons, testing::UnorderedElementsAre("unknown-error"));

    ResetUpdate({.Basic = BasicStatus::ERROR_BIT, .Engine = EngineReadyStatus::NO_PRINT_PAPER});
    EXPECT_THAT(Parser.Reasons, testing::UnorderedElementsAre("unknown-error"));

    ResetUpdate({.Basic = BasicStatus::ERROR_BIT, .Engine = EngineReadyStatus::SERVICE_CALL});
    EXPECT_THAT(Parser.Reasons, testing::UnorderedElementsAre("other-error"));

    ResetUpdate({.Basic = BasicStatus::ERROR_BIT, .Engine = EngineReadyStatus::SERVICE_CALL | EngineReadyStatus::JAM});
    EXPECT_THAT(Parser.Reasons, testing::UnorderedElementsAre("other-error"));

    Update({.Engine = EngineReadyStatus::JAM});
    EXPECT_THAT(Parser.Reasons, testing::UnorderedElementsAre("media-jam-error"));

    Update({.Engine = EngineReadyStatus::SERVICE_CALL | EngineReadyStatus::JAM});
    EXPECT_THAT(Parser.Reasons, testing::UnorderedElementsAre("other-error"));
}
