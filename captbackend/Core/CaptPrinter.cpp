#include "CaptPrinter.hpp"
#include "StatusMessage.hpp"
#include "Log.hpp"
#include <cassert>
#include <libcapt/Utility/Crop.hpp>
#include <libcapt/Utility/CropStreambuf.hpp>
#include <libcapt/Compression/ScoaStreambuf.hpp>

using namespace std::literals::chrono_literals;

static inline Capt::Utility::CropStreambuf crop(RasterStreambuf& rasterStr, Capt::Protocol::PageParams& params) noexcept {
    uint16_t lineSize = Capt::Utility::CropLineSize(params.ImageLineSize, params.PaperWidth);
    uint16_t lines = Capt::Utility::CropLinesCount(params.ImageLines, params.PaperHeight);
    Capt::Utility::CropStreambuf cropStr(rasterStr, params.ImageLineSize, params.ImageLines, lineSize, lines);
    Log::Debug() << "Cropping raster from " << params.ImageLineSize << 'x' << params.ImageLines
        << " to " << lineSize << 'x' << lines;
    params.ImageLineSize = lineSize;
    params.ImageLines = lines;
    return cropStr;
}

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
            Log::Debug() << "Calling ClearError()";
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

std::optional<Capt::Protocol::ExtendedStatus> CaptPrinter::WritePage(std::stop_token stopToken, Capt::Utility::BufferedPage& page, Capt::Utility::BufferedPage* prev) {
    Capt::Protocol::ReprintStatus reprint = Capt::Protocol::ReprintStatus::None;
    while (!stopToken.stop_requested()) {
        Capt::Utility::BufferedPage& p = (prev && reprint == Capt::Protocol::ReprintStatus::Prev) ? *prev : page;
        p.pubseekpos(0);
        this->PrepareBeforePrint(stopToken, p.PageNumber);
        if (stopToken.stop_requested()) {
            return std::nullopt;
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
            return std::nullopt;
        }
        if (status->VideoDataError() || status->FatalError()) {
            return status;
        }
        reprint = status->GetReprintStatus();
        assert(!status->Ready());
        std::this_thread::sleep_for(1s);
    }
    return std::nullopt;
}

std::optional<Capt::Protocol::ExtendedStatus> CaptPrinter::WaitLastPage(std::stop_token stopToken, Capt::Utility::BufferedPage& page) {
    while (!stopToken.stop_requested()) {
        std::this_thread::sleep_for(1s);
        auto status = this->WaitPrintEnd(stopToken);
        if (!status) {
            return std::nullopt;
        }
        if (status->VideoDataError() || status->FatalError()) {
            return status;
        } else if (status->GetReprintStatus() == Capt::Protocol::ReprintStatus::None) {
            break;
        }
        auto res = this->WritePage(stopToken, page, nullptr);
        if (res.has_value()) {
            return *res;
        }
    }
    return std::nullopt;
}

bool CaptPrinter::Print(std::stop_token stopToken, RasterStreambuf& rasterStr) {
    unsigned page = 0;
    Capt::Utility::BufferedPage prevPage;
    while (!stopToken.stop_requested()) {
        std::optional<Capt::Protocol::PageParams> params = rasterStr.NextPage();
        if (!params) {
            break;
        }
        Capt::Utility::CropStreambuf cropStr = crop(rasterStr, *params);
        Capt::Compression::ScoaStreambuf ss(cropStr, params->ImageLineSize, params->ImageLines);
        Capt::Utility::BufferedPage currPage(page, *params, &ss);
        reporter.Page(page + 1);

        auto res = this->WritePage(stopToken, currPage, page == 0 ? nullptr : &prevPage);
        if (res.has_value()) {
            Log::Debug() << "WritePage failed: " << *res;
            Log::Critical() << "Failed to write page (" << StatusMessage(*res) << ')';
            return false;
        }
        prevPage = std::move(currPage);
        page++;
    }

    Log::Info() << "Waiting for last page...";
    if (page != 0) {
        auto res = this->WaitLastPage(stopToken, prevPage);
        if (res.has_value()) {
            Log::Debug() << "WaitLastPage failed: " << *res;
            Log::Critical() << "Failed to write page (" << StatusMessage(*res) << ')';
            return false;
        }
    }
    return true;
}

bool CaptPrinter::Clean(std::stop_token stopToken) {
    while (!stopToken.stop_requested()) {
        this->PrepareBeforePrint(stopToken, 0);
        std::this_thread::sleep_for(1s); // Engine status delay
        this->Cleaning();
        Log::Info() << "Cleaning...";
        std::this_thread::sleep_for(2s);

        Capt::Protocol::ExtendedStatus status = this->GetStatus();
        if (status.FatalError()) {
            Log::Critical() << "Unknown fatal error";
            return false;
        }
        if ((status.Engine & Capt::Protocol::EngineReadyStatus::CLEANING) == 0) {
            Log::Warning() << "Cleaning failed (" << StatusMessage(status) << ')';
            continue;
        }
        this->WaitPrintEnd(stopToken);
        break;
    }
    return true;
}
