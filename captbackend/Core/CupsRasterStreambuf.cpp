#include "CupsRasterStreambuf.hpp"
#include "CupsRasterError.hpp"

using int_type = CupsRasterStreambuf::int_type;

int_type CupsRasterStreambuf::underflow() {
    if (this->gptr() < this->egptr()) {
        return traits_type::to_int_type(*this->gptr());
    }
    if (this->line == this->Header.cupsHeight) {
        return traits_type::eof();
    }
    std::size_t read = cupsRasterReadPixels(&this->raster, reinterpret_cast<unsigned char*>(this->lineBuffer.data()), this->lineBuffer.size());
    if (read != 0 && read != this->lineBuffer.size()) {
        throw CupsRasterError("failed to read raster data");
    }
    this->line++;
    if (this->line == this->Header.cupsHeight && read == 0) {
        throw CupsRasterError("unexpected EOF");
    }
    char_type* start = this->lineBuffer.data();
    char_type* end = start + read;
    this->setg(start, start, end);
    return traits_type::to_int_type(*this->gptr());
}

CupsRasterStreambuf::CupsRasterStreambuf(const cups_page_header2_t& header, cups_raster_t& raster)
    : raster(raster), lineBuffer(header.cupsBytesPerLine), Header(header) {}
