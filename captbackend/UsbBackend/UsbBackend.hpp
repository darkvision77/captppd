#ifndef _CAPTBACKEND_USB_BACKEND_USB_BACKEND_HPP_
#define _CAPTBACKEND_USB_BACKEND_USB_BACKEND_HPP_

#include "UsbPrinter.hpp"
#include <libusb.h>
#include <vector>

typedef std::unique_ptr<libusb_context, decltype(&libusb_exit)> libusb_context_ptr;

class UsbBackend {
private:
    libusb_context_ptr context;
public:
    UsbBackend() noexcept;

    bool Init() noexcept;
    std::vector<UsbPrinter> GetPrinters();
};

#endif
