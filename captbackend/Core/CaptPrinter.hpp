#ifndef _CAPTBACKEND_CORE_CAPT_PRINTER_HPP_
#define _CAPTBACKEND_CORE_CAPT_PRINTER_HPP_

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

    Capt::Protocol::ExtendedStatus GetStatus() override;

    Capt::Protocol::ExtendedStatus WaitReady(StopTokenType stopToken);
    void PrepareBeforePrint(StopTokenType stopToken, unsigned page);

    // Has value if error
    std::optional<Capt::Protocol::ExtendedStatus> WritePage(StopTokenType stopToken, Capt::Utility::BufferedPage& page, Capt::Utility::BufferedPage* prev);

    // Has value if error
    std::optional<Capt::Protocol::ExtendedStatus> WaitLastPage(StopTokenType stopToken, Capt::Utility::BufferedPage& page, unsigned pageNum);
    std::optional<Capt::Protocol::ExtendedStatus> WaitLastPage(StopTokenType stopToken, Capt::Utility::BufferedPage& page);

    bool Print(StopTokenType stopToken, RasterStreambuf& rasterStr);
    bool Clean(StopTokenType stopToken);
};

#endif
