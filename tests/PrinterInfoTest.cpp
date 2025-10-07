#include "Core/PrinterInfo.hpp"
#include "Config.hpp"
#include <gtest/gtest.h>
#include <sstream>
#include <string_view>
#include <utility>

TEST(PrinterInfoTest, Uri) {
    PrinterInfo info{
        .DeviceId = "MFG:Canon;MDL:LBP3200;CMD:CAPT;VER:1.0;CLS:PRINTER;DES:Canon LBP3200",
        .Manufacturer = "Canon",
        .Model = "LBP3200",
        .Description = "Canon LBP3200",
        .Serial = "98765432",
        .CommandSet = "CAPT",
        .CmdVersion = "1.0",
    };

    std::ostringstream ss;
    info.WriteUri(ss);
    EXPECT_TRUE(info.HasUri(ss.str()));
    EXPECT_TRUE(info.HasUri(CAPTBACKEND_NAME "://Canon/LBP3200?serial=98765432"));
    EXPECT_TRUE(info.HasUri(CAPTBACKEND_NAME "://Canon/LBP3200?somevar=test&serial=98765432"));
    EXPECT_TRUE(info.HasUri(CAPTBACKEND_NAME "://Canon/LBP3200?serial=98765432&somevar=test"));

    EXPECT_FALSE(info.HasUri(CAPTBACKEND_NAME "://Canon/LBP3200"));
    EXPECT_FALSE(info.HasUri(CAPTBACKEND_NAME "://Canon/LBP3201?serial=98765432"));
    EXPECT_FALSE(info.HasUri(CAPTBACKEND_NAME "://Can0n/LBP3200?serial=98765432"));

    EXPECT_FALSE(info.HasUri(CAPTBACKEND_NAME "://Canon/LBP3200?"));
    EXPECT_FALSE(info.HasUri(CAPTBACKEND_NAME "://Can0n/LBP3200?serial="));
    EXPECT_FALSE(info.HasUri(CAPTBACKEND_NAME "://Can0n/LBP3200?serial"));
    EXPECT_FALSE(info.HasUri(CAPTBACKEND_NAME "://Canon/LBP3200?serial=98765432x"));
    EXPECT_FALSE(info.HasUri(CAPTBACKEND_NAME "://Canon/LBP3200?serial=x98765432"));
    EXPECT_FALSE(info.HasUri(CAPTBACKEND_NAME "://Canon/LBP3200?serial 98765432"));
    EXPECT_FALSE(info.HasUri(CAPTBACKEND_NAME "://Canon/LBP3200?serial98765432"));
}

TEST(PrinterInfoTest, Parse) {
    PrinterInfo expected{
        .DeviceId = "MFG:Canon;MDL:LBP3200;CMD:CAPT;VER:1.0;CLS:PRINTER;DES:Canon LBP3200",
        .Manufacturer = "Canon",
        .Model = "LBP3200",
        .Description = "Canon LBP3200",
        .Serial = "98765432",
        .CommandSet = "CAPT",
        .CmdVersion = "1.0",
    };

    PrinterInfo info = PrinterInfo::Parse(expected.DeviceId, expected.Serial);
    EXPECT_EQ(info.DeviceId, expected.DeviceId);
    EXPECT_EQ(info.Manufacturer, expected.Manufacturer);
    EXPECT_EQ(info.Model, expected.Model);
    EXPECT_EQ(info.Description, expected.Description);
    EXPECT_EQ(info.Serial, expected.Serial);
    EXPECT_EQ(info.CommandSet, expected.CommandSet);
    EXPECT_EQ(info.CmdVersion, expected.CmdVersion);
}

TEST(PrinterInfoTest, ParseLongKeys) {
    PrinterInfo expected{
        .DeviceId = "MANUFACTURER:Canon;MODEL:LBP3200;COMMAND SET:CAPT;VER:1.0;CLS:PRINTER;DESCRIPTION:Canon LBP3200",
        .Manufacturer = "Canon",
        .Model = "LBP3200",
        .Description = "Canon LBP3200",
        .Serial = "98765432",
        .CommandSet = "CAPT",
        .CmdVersion = "1.0",
    };

    PrinterInfo info = PrinterInfo::Parse(expected.DeviceId, expected.Serial);
    EXPECT_EQ(info.DeviceId, expected.DeviceId);
    EXPECT_EQ(info.Manufacturer, expected.Manufacturer);
    EXPECT_EQ(info.Model, expected.Model);
    EXPECT_EQ(info.Description, expected.Description);
    EXPECT_EQ(info.Serial, expected.Serial);
    EXPECT_EQ(info.CommandSet, expected.CommandSet);
    EXPECT_EQ(info.CmdVersion, expected.CmdVersion);
}

TEST(PrinterInfoTest, ParseInvalid) {
    PrinterInfo expected{
        .DeviceId = "test",
        .Manufacturer = "",
        .Model = "",
        .Description = "",
        .Serial = "98765432",
        .CommandSet = "",
        .CmdVersion = "",
    };

    PrinterInfo info = PrinterInfo::Parse(expected.DeviceId, expected.Serial);
    EXPECT_EQ(info.DeviceId, expected.DeviceId);
    EXPECT_EQ(info.Manufacturer, expected.Manufacturer);
    EXPECT_EQ(info.Model, expected.Model);
    EXPECT_EQ(info.Description, expected.Description);
    EXPECT_EQ(info.Serial, expected.Serial);
    EXPECT_EQ(info.CommandSet, expected.CommandSet);
    EXPECT_EQ(info.CmdVersion, expected.CmdVersion);
}

TEST(PrinterInfoTest, IsCaptPrinter) {
    const std::string_view serial = "98765432";
    const std::pair<std::string_view, bool> cases[] = {
        { "MFG:Test;MDL:Test2;CMD:CAPT;VER:1.0;CLS:PRINTER;DES:Canon LBP3200", true },
        { "MFG:Test;MDL:Test2;CMD:CAPT;VER:1.1;CLS:PRINTER;DES:Canon LBP3200", true },
        { "MFG:Test;MDL:Test2;CMD:CAPT;VER:1;CLS:PRINTER;DES:Canon LBP3200", true },
        { "MFG:Test;MDL:Test2;CMD:CAPT;VER:0;CLS:PRINTER;DES:Canon LBP3200", false },
        { "MFG:Test;MDL:Test2;CMD:CAPT;VER:;CLS:PRINTER;DES:Canon LBP3200", false },
        { "MFG:Canon;MDL:LBP3200;CMD:CAPT;VER:2;CLS:PRINTER;DES:Canon LBP3200", false },
        { "MFG:Canon;MDL:LBP3200;CMD:CAPT;VER:2.0;CLS:PRINTER;DES:Canon LBP3200", false },
        { "MFG:Canon;MDL:LBP3200;CMD:TEST;VER:1.0;CLS:PRINTER;DES:Canon LBP3200", false },
    };

    for(const auto& [devId, expected] : cases) {
        PrinterInfo info = PrinterInfo::Parse(devId, serial);
        ASSERT_EQ(info.DeviceId, devId);
        ASSERT_EQ(info.Serial, serial);
        EXPECT_EQ(info.IsCaptPrinter(), expected);
    }
}
