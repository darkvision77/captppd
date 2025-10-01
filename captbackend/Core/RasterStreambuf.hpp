#ifndef _CAPTBACKEND_CORE_RASTER_STREAMBUF_HPP_
#define _CAPTBACKEND_CORE_RASTER_STREAMBUF_HPP_

#include <streambuf>
#include <optional>
#include <libcapt/Protocol/PageParams.hpp>

class RasterStreambuf : public virtual std::streambuf {
public:
    virtual ~RasterStreambuf() noexcept = default;
    virtual std::optional<Capt::Protocol::PageParams> NextPage() = 0;
};

#endif
