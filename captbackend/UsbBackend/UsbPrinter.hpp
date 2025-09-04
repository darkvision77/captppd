#ifndef _CAPTBACKEND_USB_BACKEND_USB_DEVICE_HPP_
#define _CAPTBACKEND_USB_BACKEND_USB_DEVICE_HPP_

#include <libusb.h>
#include <memory>
#include <string>

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

    std::string getStringDescriptor(uint8_t idx);
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

    bool Open() noexcept;
    bool Claim() noexcept;
    void Release() noexcept;
    void Close() noexcept;

    bool DetachKernelDriver() noexcept;
    void ReattachKernelDriver() noexcept;

    std::string GetDeviceId();

    inline std::string GetSerial() {
        return this->getStringDescriptor(this->desc.iSerialNumber);
    }
};

#endif
