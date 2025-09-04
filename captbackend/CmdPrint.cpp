#include "Core/CaptPrinter.hpp"
#include "Core/CupsRasterStreambuf.hpp"
#include "Core/Log.hpp"
#include <cups/backend.h>
#include <cups/raster.h>
#include <libcapt/UnexpectedBehaviourError.hpp>
#include <libcapt/Compression/ScoaStreambuf.hpp>
#include <libcapt/Protocol/Enums.hpp>
#include <libcapt/Protocol/ExtendedStatus.hpp>
#include <libcapt/Protocol/Protocol.hpp>
#include <libcapt/Core/CaptPacket.hpp>
#include <libcapt/Utility/Crop.hpp>
#include <libcapt/Utility/CropStreambuf.hpp>
#include <stop_token>
#include <memory>

typedef std::unique_ptr<cups_raster_t, decltype(&cupsRasterClose)> cupsRasterUniquePtr;

static Capt::Protocol::PageParams makeParams(const cups_page_header2_t& header, bool crop = true) noexcept {
    Capt::Protocol::PageParams params{
        .PaperSize = static_cast<uint8_t>(header.cupsMediaType),
        .TonerDensity = static_cast<uint8_t>(header.cupsCompression),
        .Mode = static_cast<uint8_t>(0),
        .Resolution = header.HWResolution[0] == 600 ? Capt::Protocol::RES_600 : Capt::Protocol::RES_300,
        .SmoothEnable = header.cupsInteger[4] != 0,
        .TonerSaving = header.cupsInteger[5] != 0,
        .MarginLeft = static_cast<uint16_t>(header.cupsInteger[0]),
        .MarginTop = static_cast<uint16_t>(header.cupsInteger[1]),
        .ImageLineSize = static_cast<uint16_t>(header.cupsBytesPerLine),
        .ImageLines = static_cast<uint16_t>(header.cupsHeight),
        .PaperWidth = static_cast<uint16_t>(header.cupsInteger[2]),
        .PaperHeight = static_cast<uint16_t>(header.cupsInteger[3]),
    };
    if (crop) {
        params.ImageLineSize = Capt::Utility::CropLineSize(params.ImageLineSize, params.PaperWidth);
        params.ImageLines = Capt::Utility::CropLinesCount(params.ImageLines, params.PaperHeight);
    }
    return params;
}

int CmdPrint(std::stop_token stopToken, StateReporter& reporter, CaptPrinter& printer, int rasterFd) {
    cupsRasterUniquePtr ras(cupsRasterOpen(rasterFd, CUPS_RASTER_READ), cupsRasterClose);
    if (ras.get() == nullptr) {
        Log::Critical() << "Failed to open raster";
        return CUPS_BACKEND_FAILED;
    }
    Log::Info() << "Raster opened";

    unsigned page = 0;
    Capt::Utility::BufferedPage prevPage;
    while (true) {
        cups_page_header2_t header;
        if (!cupsRasterReadHeader2(ras.get(), &header)) {
            break;
        }
        if (header.cupsBitsPerPixel != 1 || header.cupsBitsPerColor != 1 || header.cupsNumColors != 1) {
            Log::Critical() << "Invalid raster format";
            return CUPS_BACKEND_FAILED;
        }
        CupsRasterStreambuf pageStr(header, *ras.get());
        Capt::Protocol::PageParams params = makeParams(header);
        Capt::Utility::CropStreambuf rasterStream(pageStr, header.cupsBytesPerLine, header.cupsHeight, params.ImageLineSize, params.ImageLines);
        Capt::Compression::ScoaStreambuf ss(rasterStream, params.ImageLineSize, params.ImageLines);
        Capt::Utility::BufferedPage currPage(page, params, &ss);

        reporter.Page(page + 1);

        if (!printer.WritePage(stopToken, currPage, page == 0 ? nullptr : &prevPage)) {
            Log::Critical() << "Failed to write page";
            return CUPS_BACKEND_FAILED;
        }
        prevPage = std::move(currPage);
        page++;
    }

    Log::Info() << "Waiting for last page...";
    if (page != 0 && !printer.WaitLastPage(stopToken, prevPage)) {
        Log::Critical() << "Failed to write page";
        return CUPS_BACKEND_FAILED;
    }
    return CUPS_BACKEND_OK;
}
