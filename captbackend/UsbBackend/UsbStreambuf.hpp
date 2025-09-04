#ifndef _CAPTBACKEND_USB_BACKEND_USB_STREAMBUF_HPP_
#define _CAPTBACKEND_USB_BACKEND_USB_STREAMBUF_HPP_

#include "UsbPrinter.hpp"
#include <streambuf>
#include <libusb.h>
#include <vector>

class UsbStreambuf : public std::streambuf {
private:
    UsbPrinter& printer;

    std::vector<char_type> rbuff;
    std::vector<char_type> wbuff;

    unsigned timeoutMs;

    int_type overflow(int_type c) override;
    int_type underflow() override;

    int sync() override;
public:
    explicit UsbStreambuf(UsbPrinter& printer, std::size_t buffSize = 65535, unsigned timeoutMs = 5000);
};

#endif
