#include "CupsRasterStreambuf.hpp"
#include "Core/Log.hpp"
#include "Core/RasterError.hpp"
#include <cassert>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <libcapt/Protocol/Enums.hpp>

using int_type = CupsRasterStreambuf::int_type;

int_type CupsRasterStreambuf::underflow() {
    assert(this->raster != nullptr);
    if (this->gptr() < this->egptr()) {
        return traits_type::to_int_type(*this->gptr());
    }
    if (this->linesRemain == 0) {
        return traits_type::eof();
    }
    std::size_t read = cupsRasterReadPixels(this->raster, reinterpret_cast<unsigned char*>(this->lineBuffer.data()), this->lineBuffer.size());
    if (read != this->lineBuffer.size()) {
        Log::Debug() << "cupsRasterReadPixels returned " << read
            << ", requested " << this->lineBuffer.size()
            << ", linesRemain=" << this->linesRemain;
        throw RasterError("unexpected EOF");
    }
    this->linesRemain--;
    char_type* start = this->lineBuffer.data();
    char_type* end = start + read;
    this->setg(start, start, end);
    return traits_type::to_int_type(*this->gptr());
}

CupsRasterStreambuf::~CupsRasterStreambuf() noexcept {
    this->Close();
}

bool CupsRasterStreambuf::Open(const char* file) noexcept {
    assert(this->raster == nullptr);
    if (file == nullptr) {
        this->fd = STDIN_FILENO;
    } else {
        this->fd = open(file, O_RDONLY);
        if (this->fd < 0) {
            Log::Debug() << "open() failed: " << strerror(errno);
            return false;
        }
    }
    this->raster = cupsRasterOpen(this->fd, CUPS_RASTER_READ);
    return this->raster != nullptr;
}

void CupsRasterStreambuf::Close() noexcept {
    if (this->raster != nullptr) {
        cupsRasterClose(this->raster);
    }
    if (this->fd >= 0 && this->fd != STDIN_FILENO) {
        close(this->fd);
    }
}

std::optional<Capt::Protocol::PageParams> CupsRasterStreambuf::NextPage() {
    assert(this->raster != nullptr);
    assert(this->linesRemain == 0);
    cups_page_header2_t header;
    if (!cupsRasterReadHeader2(this->raster, &header)) {
        Log::Debug() << "No more pages";
        return std::nullopt;
    }
    if (header.cupsBitsPerPixel != 1 || header.cupsBitsPerColor != 1 || header.cupsNumColors != 1) {
        Log::Debug() << "Invalid raster format: cupsBitsPerPixel=" << header.cupsBitsPerPixel
            << " cupsBitsPerColor=" << header.cupsBitsPerColor
            << " cupsNumColors=" << header.cupsNumColors;
        throw RasterError("invalid raster format");
    }
    Log::Debug() << "Read header " << header.cupsBytesPerLine << 'x' << header.cupsHeight << " (" << header.cupsPageSizeName << ')';
    this->linesRemain = header.cupsHeight;
    this->lineBuffer.resize(header.cupsBytesPerLine);
    return Capt::Protocol::PageParams{
        .PaperSize = static_cast<uint8_t>(header.cupsInteger[4]),
        .TonerDensity = static_cast<uint8_t>(header.cupsCompression),
        .Mode = static_cast<uint8_t>(header.cupsMediaType),
        .Resolution = header.HWResolution[0] == 600 ? Capt::Protocol::RES_600 : Capt::Protocol::RES_300,
        .SmoothEnable = header.cupsInteger[5] != 0,
        .TonerSaving = header.cupsInteger[6] != 0,
        .MarginLeft = static_cast<uint16_t>(header.cupsInteger[0]),
        .MarginTop = static_cast<uint16_t>(header.cupsInteger[1]),
        .ImageLineSize = static_cast<uint16_t>(header.cupsBytesPerLine),
        .ImageLines = static_cast<uint16_t>(header.cupsHeight),
        .PaperWidth = static_cast<uint16_t>(header.cupsInteger[2]),
        .PaperHeight = static_cast<uint16_t>(header.cupsInteger[3]),
    };
}
