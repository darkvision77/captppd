#include "UsbPrinter.hpp"
#include "Core/Log.hpp"
#include "UsbError.hpp"
#include <iomanip>
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

int UsbPrinter::open() noexcept {
    if (this->handle.get() != nullptr) {
        return LIBUSB_SUCCESS;
    }
    libusb_device_handle* handle;
    int err = libusb_open(this->dev.get(), &handle);
    if (err != LIBUSB_SUCCESS) {
        Log::Debug() << "libusb_open failed: " << libusb_error_name(err);
    } else {
        this->handle.reset(handle);
    }
    return err;
}

void UsbPrinter::claim() {
    assert(this->handle.get() != nullptr);
    int err = libusb_claim_interface(this->handle.get(), this->alt.bInterfaceNumber);
    if (err != LIBUSB_SUCCESS) {
        Log::Debug() << "libusb_claim_interface failed: " << libusb_error_name(err);
        throw UsbError("failed to claim interface", err);
    }
    this->interfaceClaimed = true;
}

void UsbPrinter::detachKernelDriver() {
    assert(this->handle.get() != nullptr);
    int err = libusb_kernel_driver_active(this->handle.get(), this->alt.bInterfaceNumber);
    if (err == 0) {
        Log::Debug() << "Kernel driver is not attached";
    } else if (err == 1) {
        err = libusb_detach_kernel_driver(this->handle.get(), this->alt.bInterfaceNumber);
        if (err != LIBUSB_SUCCESS) {
            Log::Debug() << "libusb_detach_kernel_driver failed: " << libusb_error_name(err);
            throw UsbError("failed to detach kernel driver", err);
        }
        this->kernelDriverDetached = true;
        Log::Debug() << "Kernel driver detached";
    } else if (err == LIBUSB_ERROR_NOT_SUPPORTED) {
        Log::Debug() << "libusb_kernel_driver_active is not supported";
    } else {
        Log::Debug() << "libusb_kernel_driver_active failed: " << libusb_error_name(err);
        throw UsbError("failed to check kernel driver", err);
    }
}

void UsbPrinter::setConfig(uint8_t value) {
    assert(this->handle.get() != nullptr);
    if (this->desc.bNumConfigurations == 1) {
        return;
    }
    int err = libusb_set_configuration(this->handle.get(), value);
    if (err != LIBUSB_SUCCESS) {
        Log::Debug() << "libusb_set_configuration failed: " << libusb_error_name(err);
        throw UsbError("failed to set device configuration", err);
    }
    Log::Debug() << "Device configuration set to " << value;
}

void UsbPrinter::Open() {
    int err = this->open();
    if (err != LIBUSB_SUCCESS) {
        throw UsbError("failed to open device", err);
    }
    this->detachKernelDriver();
    this->setConfig(this->config->bConfigurationValue);
    this->claim();
}

void UsbPrinter::release() noexcept {
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

void UsbPrinter::reattachKernelDriver() noexcept {
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

void UsbPrinter::Close() noexcept {
    if (this->handle.get() == nullptr) {
        return;
    }
    if (this->interfaceClaimed) {
        this->release();
    }
    if (this->kernelDriverDetached) {
        this->reattachKernelDriver();
    }
    this->handle.reset();
}

void UsbPrinter::reset() noexcept {
    assert(this->handle.get() != nullptr);
    int err = libusb_reset_device(this->handle.get());
    if (err != LIBUSB_SUCCESS) {
        Log::Debug() << "libusb_reset_device failed: " << libusb_error_name(err);
    } else {
        Log::Debug() << "Reset requested";
    }
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

std::string UsbPrinter::getDeviceId() {
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

std::optional<PrinterInfo> UsbPrinter::GetPrinterInfo() {
    bool opened = this->handle.get() == nullptr;
    if (opened) {
        int err = this->open();
        if (err != LIBUSB_SUCCESS) {
            Log::Debug() << "Failed to open device " << std::hex << std::setfill('0')
                << std::setw(4) << this->VendorId() << ':' << std::setw(4) << this->VendorId()
                << " (" << libusb_error_name(err) << "), skipping";
            return std::nullopt;
        }
    }
    std::string serial = this->getStringDescriptor(this->desc.iSerialNumber);
    PrinterInfo info = PrinterInfo::Parse(this->getDeviceId(), serial);
    if (opened) {
        this->Close();
    }
    return info;
}
