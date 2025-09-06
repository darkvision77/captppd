#ifndef _CAPTBACKEND_USB_BACKEND_USB_ERROR_HPP_
#define _CAPTBACKEND_USB_BACKEND_USB_ERROR_HPP_

#include <stdexcept>
#include <libusb.h>

class UsbError : public std::runtime_error {
public:
	int Errcode;

	explicit UsbError(std::string message, int errcode) noexcept : std::runtime_error(message), Errcode(errcode) {}

	const char* StrErrcode() const noexcept {
		return libusb_strerror(this->Errcode);
	}
};

#endif
