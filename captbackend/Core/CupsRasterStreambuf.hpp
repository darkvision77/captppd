#ifndef _CAPTBACKEND_CORE_CUPS_RASTER_STREAMBUF_HPP_
#define _CAPTBACKEND_CORE_CUPS_RASTER_STREAMBUF_HPP_

#include <cups/raster.h>
#include <streambuf>
#include <vector>

class CupsRasterStreambuf : public std::streambuf {
private:
    unsigned line = 0;
    cups_raster_t& raster;
    std::vector<char_type> lineBuffer;

    int_type underflow() override;
public:
    const cups_page_header2_t& Header;

    explicit CupsRasterStreambuf(const cups_page_header2_t& header, cups_raster_t& raster);
};

#endif
