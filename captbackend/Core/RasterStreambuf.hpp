#pragma once
#include <streambuf>
#include <optional>
#include <libcapt/Protocol/PageParams.hpp>

class RasterStreambuf : public virtual std::streambuf {
public:
    virtual ~RasterStreambuf() noexcept = default;
    virtual std::optional<Capt::PageParams> NextPage() = 0;
};
