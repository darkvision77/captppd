#include "CaptPrinter.hpp"
#include "StatusMessage.hpp"
#include "Log.hpp"
#include <cassert>

using namespace std::literals::chrono_literals;

CaptPrinter::CaptPrinter(std::iostream& stream, StateReporter& reporter) noexcept
    : Capt::BasicCaptPrinter(stream), reporter(reporter) {}

Capt::Protocol::ExtendedStatus CaptPrinter::GetStatus() {
    Capt::Protocol::ExtendedStatus status = this->Capt::BasicCaptPrinter::GetStatus();
    this->reporter.Update(status);
    return status;
}

Capt::Protocol::ExtendedStatus CaptPrinter::WaitReady(std::stop_token stopToken) {
    Capt::Protocol::ExtendedStatus status = this->GetStatus();
    while (!stopToken.stop_requested() && !status.Ready()) {
        if (status.ClearErrorNeeded()) {
            Log::Debug() << "Cleaning error...";
            this->ClearError(&status);
        }
        Log::Info() << "Stopped (" << StatusMessage(status) << ')';
        std::this_thread::sleep_for(1s);
        status = this->GetStatus();
    }
    return status;
}

void CaptPrinter::PrepareBeforePrint(std::stop_token stopToken, unsigned page) {
    while (true) {
        Capt::Protocol::ExtendedStatus status = this->WaitReady(stopToken);
        if (stopToken.stop_requested()) {
            return;
        }
        assert(status.Ready());
        if (!status.Online() || status.Start != page) {
            if (!this->GoOnline(page)) {
                Log::Warning() << "GoOnline failed, retrying...";
                std::this_thread::sleep_for(1s);
                continue;
            }
        }
        break;
    }
}

bool CaptPrinter::WritePage(std::stop_token stopToken, Capt::Utility::BufferedPage& page, Capt::Utility::BufferedPage* prev) {
    Capt::Protocol::ReprintStatus reprint = Capt::Protocol::ReprintStatus::None;
    while (!stopToken.stop_requested()) {
        Capt::Utility::BufferedPage& p = (prev && reprint == Capt::Protocol::ReprintStatus::Prev) ? *prev : page;
        p.pubseekpos(0);
        this->PrepareBeforePrint(stopToken, p.PageNumber);
        if (stopToken.stop_requested()) {
            return true;
        }
        if (reprint != Capt::Protocol::ReprintStatus::None) {
            Log::Info() << "Retrying page " << (p.PageNumber + 1);
        } else {
            Log::Info() << "Writing page " << (p.PageNumber + 1);
        }
        if (this->WriteVideoData(stopToken, p.Params, p)) {
            if (prev && reprint == Capt::Protocol::ReprintStatus::Prev) {
                reprint = Capt::Protocol::ReprintStatus::None;
                continue;
            }
            break;
        }
        auto status = this->WaitPrintEnd(stopToken);
        if (!status) {
            return true;
        }
        if (status->VideoDataError()) {
            return false;
        }
        reprint = status->GetReprintStatus();
        assert(!status->Ready());
        std::this_thread::sleep_for(1s);
    }
    return true;
}

bool CaptPrinter::WaitLastPage(std::stop_token stopToken, Capt::Utility::BufferedPage& page) {
    while (!stopToken.stop_requested()) {
        std::this_thread::sleep_for(1s);
        auto status = this->WaitPrintEnd(stopToken);
        if (!status) {
            return true;
        }
        if (status->VideoDataError()) {
            return false;
        } else if (status->GetReprintStatus() == Capt::Protocol::ReprintStatus::None) {
            break;
        }
        if (!this->WritePage(stopToken, page, nullptr)) {
            return false;
        }
    }
    return true;
}
