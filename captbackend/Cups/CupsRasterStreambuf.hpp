#ifndef _CAPTBACKEND_CUPS_CUPS_RASTER_STREAMBUF_HPP_
#define _CAPTBACKEND_CUPS_CUPS_RASTER_STREAMBUF_HPP_

#include "Core/RasterStreambuf.hpp"
#include <cups/raster.h>
#include <streambuf>
#include <unistd.h>
#include <vector>

class CupsRasterStreambuf : public RasterStreambuf {
private:
    int fd = STDIN_FILENO;
    unsigned linesRemain = 0;
    cups_raster_t* raster = nullptr;
    std::vector<char_type> lineBuffer;

    int_type underflow() override;
public:
    CupsRasterStreambuf() noexcept = default;
    ~CupsRasterStreambuf() noexcept override;

    bool Open(const char* file = nullptr) noexcept;
    void Close() noexcept;

    std::optional<Capt::Protocol::PageParams> NextPage() override;
};

#endif
