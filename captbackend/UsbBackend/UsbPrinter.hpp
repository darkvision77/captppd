#ifndef _CAPTBACKEND_USB_BACKEND_USB_DEVICE_HPP_
#define _CAPTBACKEND_USB_BACKEND_USB_DEVICE_HPP_

#include "Core/PrinterInfo.hpp"
#include <libusb.h>
#include <memory>
#include <string>
#include <optional>

typedef std::unique_ptr<libusb_device, decltype(&libusb_unref_device)> libusb_device_ptr;
typedef std::unique_ptr<libusb_device_handle, decltype(&libusb_close)> libusb_device_handle_ptr;
typedef std::unique_ptr<libusb_config_descriptor, decltype(&libusb_free_config_descriptor)> libusb_config_descriptor_ptr;

class UsbStreambuf;

class UsbPrinter {
friend UsbStreambuf;
private:
    libusb_device_ptr dev;
    libusb_device_handle_ptr handle;
    libusb_device_descriptor desc;
    libusb_config_descriptor_ptr config;
    libusb_interface_descriptor alt;
    uint8_t readEp;
    uint8_t writeEp;
    bool kernelDriverDetached = false;
    bool interfaceClaimed = false;

    int open() noexcept;
    void claim();
    void detachKernelDriver();
    void reset() noexcept;
    void release() noexcept;
    void reattachKernelDriver() noexcept;
    void setConfig(uint8_t value);

    std::string getStringDescriptor(uint8_t idx);
    std::string getDeviceId();
    std::string getSerial();
public:
    explicit UsbPrinter(
        libusb_device_ptr dev,
        const libusb_device_descriptor& desc,
        libusb_config_descriptor_ptr config,
        const libusb_interface_descriptor& alt,
        uint8_t readEp, uint8_t writeEp
    ) noexcept;
    ~UsbPrinter() noexcept;

    UsbPrinter(UsbPrinter&& other) noexcept = default;

    inline uint16_t VendorId() const noexcept {
        return this->desc.idVendor;
    }

    inline uint16_t ProductId() const noexcept {
        return this->desc.idProduct;
    }

    void Open();
    void Close() noexcept;

    std::optional<PrinterInfo> GetPrinterInfo();
};

#endif
