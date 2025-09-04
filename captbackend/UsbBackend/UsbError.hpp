#ifndef _CAPTBACKEND_USB_BACKEND_USB_ERROR_HPP_
#define _CAPTBACKEND_USB_BACKEND_USB_ERROR_HPP_

#include <stdexcept>
#include <libusb.h>

class UsbError : public std::runtime_error {
public:
	explicit UsbError(int errcode) noexcept : std::runtime_error(libusb_strerror(errcode)) {}
};

#endif
