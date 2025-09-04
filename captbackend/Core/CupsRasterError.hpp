#ifndef _CAPTBACKEND_CORE_CUPS_RASTER_ERROR_HPP_
#define _CAPTBACKEND_CORE_CUPS_RASTER_ERROR_HPP_

#include <stdexcept>

class CupsRasterError : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

#endif
