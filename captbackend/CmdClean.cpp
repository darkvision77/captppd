#include "Core/CaptPrinter.hpp"
#include "Core/Log.hpp"
#include "Core/StatusMessage.hpp"
#include <cups/backend.h>
#include <stop_token>

using namespace std::literals::chrono_literals;

int CmdClean(std::stop_token stopToken, CaptPrinter& printer) {
    while (!stopToken.stop_requested()) {
        printer.PrepareBeforePrint(stopToken, 0);
        std::this_thread::sleep_for(1s); // Engine status delay
        printer.Cleaning();
        Log::Info() << "Cleaning...";
        std::this_thread::sleep_for(2s);

        Capt::Protocol::ExtendedStatus status = printer.GetStatus();
        if (status.FatalError()) {
            Log::Critical() << "Unknown fatal error";
            return CUPS_BACKEND_FAILED;
        }
        if ((status.Engine & Capt::Protocol::EngineReadyStatus::CLEANING) == 0) {
            Log::Warning() << "Cleaning failed (" << StatusMessage(status) << ')';
            continue;
        }
        printer.WaitPrintEnd(stopToken);
        break;
    }
    return CUPS_BACKEND_OK;
}
