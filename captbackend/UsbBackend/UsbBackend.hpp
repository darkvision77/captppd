#pragma once
#include "UsbPrinter.hpp"
#include <libusb.h>
#include <vector>

using libusb_context_ptr = std::unique_ptr<libusb_context, decltype(&libusb_exit)>;

class UsbBackend {
private:
    libusb_context_ptr context;
public:
    UsbBackend() noexcept;

    void Init();
    [[nodiscard]] std::vector<UsbPrinter> GetPrinters();
};
