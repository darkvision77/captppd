#pragma once
#include <stdexcept>
#include <libusb.h>

class UsbError : public std::runtime_error {
public:
    int Errcode;

    explicit UsbError(const char* message, int errcode = LIBUSB_ERROR_OTHER) noexcept : std::runtime_error(message), Errcode(errcode) {}

    [[nodiscard]] const char* StrErrcode() const noexcept {
        return libusb_strerror(this->Errcode);
    }
};
