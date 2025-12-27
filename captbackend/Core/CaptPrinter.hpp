#pragma once
#include "RasterStreambuf.hpp"
#include "StateReporter.hpp"
#include "StopToken.hpp"
#include <libcapt/BasicCaptPrinter.hpp>
#include <libcapt/Utility/BufferedPage.hpp>
#include <iostream>

class CaptPrinter : public Capt::BasicCaptPrinter<StopToken> {
private:
    StateReporter& reporter;
public:
    explicit CaptPrinter(std::iostream& stream, StateReporter& reporter) noexcept;

    Capt::ExtendedStatus GetStatus() override;

    Capt::ExtendedStatus WaitReady(StopTokenType stopToken);
    void PrepareBeforePrint(StopTokenType stopToken, unsigned page);

    // Has value if error
    std::optional<Capt::ExtendedStatus> WritePage(StopTokenType stopToken, Capt::Utility::BufferedPage& page, Capt::Utility::BufferedPage* prev);

    // Has value if error
    std::optional<Capt::ExtendedStatus> WaitLastPage(StopTokenType stopToken, Capt::Utility::BufferedPage& page);

    bool Print(StopTokenType stopToken, RasterStreambuf& rasterStr);
    bool Clean(StopTokenType stopToken);
};
