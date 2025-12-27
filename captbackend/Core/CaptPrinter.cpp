#include "CaptPrinter.hpp"
#include "StatusMessage.hpp"
#include "Log.hpp"
#include <cassert>
#include <libcapt/Utility/Crop.hpp>
#include <libcapt/Utility/CropStreambuf.hpp>
#include <libcapt/Compression/ScoaStreambuf.hpp>

using namespace std::literals::chrono_literals;

static inline Capt::Utility::CropStreambuf crop(RasterStreambuf& rasterStr, Capt::PageParams& params) noexcept {
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
    : Capt::BasicCaptPrinter<StopTokenType>(stream), reporter(reporter) {}

Capt::ExtendedStatus CaptPrinter::GetStatus() {
    Capt::ExtendedStatus status = this->Capt::BasicCaptPrinter<StopTokenType>::GetStatus();
    this->reporter.Update(status);
    return status;
}

Capt::ExtendedStatus CaptPrinter::WaitReady(StopTokenType stopToken) {
    Capt::ExtendedStatus status = this->GetStatus();
    while (!stopToken.stop_requested() && !status.Ready()) {
        if (status.ClearErrorNeeded()) {
            Log::Debug() << "Calling ClearError()";
            Log::Debug() << "Status is " << status;
            this->ClearError(&status);
        }
        Log::Info() << "Stopped (" << StatusMessage(status) << ')';
        std::this_thread::sleep_for(1s);
        status = this->GetStatus();
    }
    return status;
}

void CaptPrinter::PrepareBeforePrint(StopTokenType stopToken, unsigned page) {
    while (true) {
        Capt::ExtendedStatus status = this->WaitReady(stopToken);
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

// Has value if error
std::optional<Capt::ExtendedStatus> CaptPrinter::WritePage(StopTokenType stopToken, Capt::Utility::BufferedPage& page, Capt::Utility::BufferedPage* prev) {
    Capt::ReprintStatus reprint = Capt::ReprintStatus::None;
    while (!stopToken.stop_requested()) {
        Capt::Utility::BufferedPage& p = (prev && reprint == Capt::ReprintStatus::Prev) ? *prev : page;
        p.pubseekpos(0);
        this->PrepareBeforePrint(stopToken, p.PageNumber);
        if (stopToken.stop_requested()) {
            return std::nullopt;
        }
        if (reprint != Capt::ReprintStatus::None) {
            Log::Info() << "Retrying page " << (p.PageNumber + 1);
        } else {
            Log::Info() << "Writing page " << (p.PageNumber + 1);
        }
        if (this->WriteVideoData(stopToken, p.Params, p)) {
            if (prev && reprint == Capt::ReprintStatus::Prev) {
                reprint = Capt::ReprintStatus::None;
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

// Has value if error
std::optional<Capt::ExtendedStatus> CaptPrinter::WaitLastPage(StopTokenType stopToken, Capt::Utility::BufferedPage& page) {
    while (!stopToken.stop_requested()) {
        std::this_thread::sleep_for(1s);
        auto status = this->WaitPrintEnd(stopToken);
        if (!status) {
            return std::nullopt;
        }
        if (status->VideoDataError() || status->FatalError()) {
            return status;
        } else if (status->GetReprintStatus() == Capt::ReprintStatus::None) {
            break;
        }
        auto res = this->WritePage(stopToken, page, nullptr);
        if (res.has_value()) {
            return *res;
        }
    }
    return std::nullopt;
}

bool CaptPrinter::Print(StopTokenType stopToken, RasterStreambuf& rasterStr) {
    unsigned page = 0;
    Capt::Utility::BufferedPage prevPage;
    Capt::Compression::ScoaStreambuf ss;
    while (!stopToken.stop_requested()) {
        std::optional<Capt::PageParams> params = rasterStr.NextPage();
        if (!params) {
            break;
        }
        Capt::Utility::CropStreambuf cropStr = crop(rasterStr, *params);
        ss.Reset(cropStr, params->ImageLineSize, params->ImageLines);
        Capt::Utility::BufferedPage currPage(page, *params, &ss);
        reporter.Page(page + 1);
        Log::Debug() << "Writing page params: ImageSize=" << static_cast<int>(params->ImageLineSize)
            << 'x' << static_cast<int>(params->ImageLines)
            << " PaperSize=" << static_cast<int>(params->PaperWidth) << 'x' << static_cast<int>(params->PaperHeight)
            << " (" << static_cast<int>(params->PaperSize)
            << ") MarginLeft=" << static_cast<int>(params->MarginLeft) << " MarginTop=" << static_cast<int>(params->MarginTop)
            << " TonerDensity=" << static_cast<int>(params->TonerDensity) << " Mode=" << static_cast<int>(params->Mode);

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
    Log::Debug() << "Status after CaptPrinter::Print(): " << this->GetStatus();
    return true;
}

bool CaptPrinter::Clean(StopTokenType stopToken) {
    while (!stopToken.stop_requested()) {
        this->PrepareBeforePrint(stopToken, 0);
        std::this_thread::sleep_for(1s); // Manual slot delay
        this->Cleaning();
        Log::Info() << "Cleaning...";
        std::this_thread::sleep_for(2s);

        Capt::ExtendedStatus status = this->GetStatus();
        if (status.FatalError()) {
            Log::Debug() << "Clean failed: " << status;
            Log::Critical() << "Unknown fatal error";
            return false;
        }
        if ((status.Engine & Capt::EngineReadyStatus::CLEANING) == 0) {
            Log::Warning() << "Cleaning failed (" << StatusMessage(status) << ')';
            continue;
        }
        this->WaitPrintEnd(stopToken);
        break;
    }
    Log::Debug() << "Status after CaptPrinter::Clean(): " << this->GetStatus();
    return true;
}
