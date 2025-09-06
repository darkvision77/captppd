#ifndef _CAPTBACKEND_CORE_CAPT_PRINTER_HPP_
#define _CAPTBACKEND_CORE_CAPT_PRINTER_HPP_

#include "StateReporter.hpp"
#include <libcapt/BasicCaptPrinter.hpp>
#include <libcapt/Utility/BufferedPage.hpp>
#include <iostream>
#include <stop_token>

class CaptPrinter : public Capt::BasicCaptPrinter {
private:
    StateReporter& reporter;
public:
    explicit CaptPrinter(std::iostream& stream, StateReporter& reporter) noexcept;
    virtual ~CaptPrinter();

    Capt::Protocol::ExtendedStatus GetStatus() override;

    Capt::Protocol::ExtendedStatus WaitReady(std::stop_token stopToken);
    void PrepareBeforePrint(std::stop_token stopToken, unsigned page);
    bool WritePage(std::stop_token stopToken, Capt::Utility::BufferedPage& page, Capt::Utility::BufferedPage* prev);
    bool WaitLastPage(std::stop_token stopToken, Capt::Utility::BufferedPage& page);
};

#endif
