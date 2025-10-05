#ifndef _CAPTBACKEND_CORE_CAPT_PRINTER_HPP_
#define _CAPTBACKEND_CORE_CAPT_PRINTER_HPP_

#include "RasterStreambuf.hpp"
#include "StateReporter.hpp"
#include <libcapt/BasicCaptPrinter.hpp>
#include <libcapt/Utility/BufferedPage.hpp>
#include <iostream>
#include <stop_token>

class CaptPrinter : public Capt::BasicCaptPrinter<std::stop_token> {
private:
    StateReporter& reporter;
public:
    explicit CaptPrinter(std::iostream& stream, StateReporter& reporter) noexcept;

    Capt::Protocol::ExtendedStatus GetStatus() override;

    Capt::Protocol::ExtendedStatus WaitReady(std::stop_token stopToken);
    void PrepareBeforePrint(std::stop_token stopToken, unsigned page);
    std::optional<Capt::Protocol::ExtendedStatus> WritePage(std::stop_token stopToken, Capt::Utility::BufferedPage& page, Capt::Utility::BufferedPage* prev);
    std::optional<Capt::Protocol::ExtendedStatus> WaitLastPage(std::stop_token stopToken, Capt::Utility::BufferedPage& page);

    bool Print(std::stop_token stopToken, RasterStreambuf& rasterStr);
    bool Clean(std::stop_token stopToken);
};

#endif
