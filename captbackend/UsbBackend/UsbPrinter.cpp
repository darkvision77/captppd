#include "UsbPrinter.hpp"
#include "Core/Log.hpp"
#include <libusb.h>
#include <cassert>

UsbPrinter::UsbPrinter(
    libusb_device_ptr dev,
    const libusb_device_descriptor& desc,
    libusb_config_descriptor_ptr config,
    const libusb_interface_descriptor& alt,
    uint8_t readEp, uint8_t writeEp
) noexcept : dev(std::move(dev)), handle(nullptr, libusb_close), desc(desc), config(std::move(config)), alt(alt), readEp(readEp), writeEp(writeEp) {}

UsbPrinter::~UsbPrinter() noexcept {
    if (this->handle.get() != nullptr) {
        this->Close();
    }
}

bool UsbPrinter::Open() noexcept {
    if (this->handle.get() != nullptr) {
        return true;
    }
    libusb_device_handle* handle;
    int err = libusb_open(this->dev.get(), &handle);
    if (err != LIBUSB_SUCCESS) {
        Log::Debug() << "libusb_open failed: " << libusb_error_name(err);
        return false;
    }
    this->handle.reset(handle);
    return true;
}

bool UsbPrinter::Claim() noexcept {
    assert(this->handle.get() != nullptr);
    int err = libusb_claim_interface(this->handle.get(), this->alt.bInterfaceNumber);
    if (err != LIBUSB_SUCCESS) {
        Log::Debug() << "libusb_claim_interface failed: " << libusb_error_name(err);
        return false;
    }
    this->interfaceClaimed = true;
    return true;
}

void UsbPrinter::Release() noexcept {
    assert(this->handle.get() != nullptr);
    if (!this->interfaceClaimed) {
        return;
    }
    int err = libusb_release_interface(this->handle.get(), this->alt.bInterfaceNumber);
    if (err != LIBUSB_SUCCESS) {
        Log::Debug() << "libusb_release_interface failed: " << libusb_error_name(err) << ", ignoring";
    }
    this->interfaceClaimed = false;
}

void UsbPrinter::Close() noexcept {
    if (this->handle.get() == nullptr) {
        return;
    }
    if (this->interfaceClaimed) {
        this->Release();
    }
    if (this->kernelDriverDetached) {
        this->ReattachKernelDriver();
    }
    this->handle.reset();
}

std::string UsbPrinter::GetDeviceId() {
    assert(this->handle.get() != nullptr);
    unsigned char buff[1024];
    int err = libusb_control_transfer(
        this->handle.get(),
        static_cast<uint8_t>(LIBUSB_REQUEST_TYPE_CLASS) | LIBUSB_ENDPOINT_IN | LIBUSB_RECIPIENT_INTERFACE,
        0, this->config->bConfigurationValue,
        (static_cast<uint16_t>(this->alt.bInterfaceNumber) << 8) | this->alt.bAlternateSetting, buff, sizeof(buff), 5000
    );
    assert(err >= 0);
    uint16_t length = (static_cast<uint16_t>(buff[0]) << 8) | static_cast<uint16_t>(buff[1]);
    assert(length >= 2 && length+2 < 1024);
    return std::string(reinterpret_cast<char*>(buff+2), length-2);
}

std::string UsbPrinter::getStringDescriptor(uint8_t idx) {
    assert(this->handle.get() != nullptr);
	unsigned char buffer[256];
    int length = libusb_get_string_descriptor_ascii(this->handle.get(), idx, buffer, sizeof(buffer));
    if (length < 0) {
        Log::Debug() << "libusb_get_string_descriptor_ascii failed: " << libusb_error_name(length);
        return "";
    }
    return std::string(reinterpret_cast<char*>(buffer), length);
}

bool UsbPrinter::DetachKernelDriver() noexcept {
    assert(this->handle.get() != nullptr);
    int err = libusb_kernel_driver_active(this->handle.get(), this->alt.bInterfaceNumber);
    if (err == 0) {
        Log::Debug() << "Kernel driver is not attached";
        return true;
    } else if (err == 1) {
        err = libusb_detach_kernel_driver(this->handle.get(), this->alt.bInterfaceNumber);
        if (err != LIBUSB_SUCCESS) {
            Log::Debug() << "libusb_detach_kernel_driver failed: " << libusb_error_name(err);
            return false;
        }
        this->kernelDriverDetached = true;
        Log::Debug() << "Kernel driver detached";
        return true;
    }
    if (err == LIBUSB_ERROR_NOT_SUPPORTED) {
        return true;
    }
    Log::Debug() << "libusb_kernel_driver_active failed: " << libusb_error_name(err);
    return false;
}

void UsbPrinter::ReattachKernelDriver() noexcept {
    assert(this->handle.get() != nullptr);
    if (!this->kernelDriverDetached) {
        return;
    }
    int err = libusb_attach_kernel_driver(this->handle.get(), this->alt.bInterfaceNumber);
    if (err != LIBUSB_SUCCESS) {
        Log::Debug() << "libusb_attach_kernel_driver failed: " << libusb_error_name(err) << ", ignoring";
    } else {
        Log::Debug() << "Kernel driver attached";
    }
}
