#include "UsbBackend.hpp"
#include "Core/Log.hpp"
#include "UsbError.hpp"
#include <cassert>
#include <iomanip>
#include <libusb.h>
#include <optional>

typedef std::unique_ptr<libusb_device*, void(*)(libusb_device**)> libusb_device_list;

static inline std::pair<libusb_device_list, ssize_t> getDeviceList(libusb_context* ctx) {
    libusb_device** devs;
    ssize_t count = libusb_get_device_list(ctx, &devs);
    return {libusb_device_list(devs, [](auto devs) { libusb_free_device_list(devs, false); }), count};
}

static inline bool isPrinter(const libusb_interface_descriptor& alt) noexcept {
    return alt.bInterfaceClass == LIBUSB_CLASS_PRINTER && alt.bInterfaceSubClass == 1 && alt.bInterfaceProtocol == 2;
}

static std::vector<libusb_config_descriptor_ptr> getConfigs(libusb_device* dev, const libusb_device_descriptor& desc) {
    assert(dev != nullptr);
    std::vector<libusb_config_descriptor_ptr> configs;
    for (int i = 0; i < desc.bNumConfigurations; i++) {
        libusb_config_descriptor* conf;
        int err = libusb_get_config_descriptor(dev, i, &conf);
        if (err != LIBUSB_SUCCESS) {
            Log::Debug() << "libusb_get_config_descriptor failed for " << std::hex << std::setfill('0')
                << std::setw(4) << desc.idVendor << ':' << std::setw(4) << desc.idProduct << ", skipping";
            continue;
        }
        configs.emplace_back(libusb_config_descriptor_ptr(conf, libusb_free_config_descriptor));
    }
    return configs;
}

static std::optional<std::pair<uint8_t, uint8_t>> getRWEndpoints(const libusb_interface_descriptor& alt) noexcept {
    int readEp = -1;
    int writeEp = -1;
    for (int i = 0; i < alt.bNumEndpoints; i++) {
        if ((alt.endpoint[i].bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) != LIBUSB_TRANSFER_TYPE_BULK) {
            continue;
        }
        if ((alt.endpoint[i].bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT) {
            writeEp = alt.endpoint[i].bEndpointAddress;
        } else {
            readEp = alt.endpoint[i].bEndpointAddress;
        }
    }
    if (readEp == -1 || writeEp == -1) {
        return std::nullopt;
    }
    return {{static_cast<uint8_t>(readEp), static_cast<uint8_t>(writeEp)}};
}

static std::optional<libusb_interface_descriptor> findPrinterAlt(libusb_config_descriptor* conf) noexcept {
    assert(conf != nullptr);
    for (int j = 0; j < conf->bNumInterfaces; j++) {
        for (int k = 0; k < conf->interface[j].num_altsetting; k++) {
            const libusb_interface_descriptor& alt = conf->interface[j].altsetting[k];
            if (isPrinter(alt)) {
                return alt;
            }
        }
    }
    return std::nullopt;
}

UsbBackend::UsbBackend() noexcept : context(nullptr, libusb_exit) {}

void UsbBackend::Init() {
    libusb_context* ctx;
    #if defined(LIBUSB_API_VERSION) && LIBUSB_API_VERSION >= 0x0100010A
    int err = libusb_init_context(&ctx, nullptr, 0);
    #else
    int err = libusb_init(&ctx);
    #endif
    if (err != LIBUSB_SUCCESS) {
        Log::Debug() << "libusb_init_context failed: " << libusb_error_name(err);
        throw UsbError("init failed", err);
    }
    this->context.reset(ctx);
}

std::vector<UsbPrinter> UsbBackend::GetPrinters() {
    auto [devs, count] = getDeviceList(this->context.get());
    if (count < 0) {
        throw UsbError("failed to enumerate devices", count);
    }
    std::vector<UsbPrinter> printers;
    for (ssize_t i = 0; i < count; i++) {
        libusb_device_ptr dev(devs.get()[i], libusb_unref_device);
        libusb_device_descriptor desc;

        int err = libusb_get_device_descriptor(dev.get(), &desc);
        if (err != LIBUSB_SUCCESS) {
            Log::Debug() << "libusb_get_device_descriptor failed: " << libusb_error_name(err) << ", skipping";
            continue;
        }

        auto configs = getConfigs(dev.get(), desc);
        for (auto& conf : configs) {
            auto alt = findPrinterAlt(conf.get());
            if (!alt) {
                continue;
            }
            auto rwEp = getRWEndpoints(*alt);
            if (!rwEp) {
                continue;
            }
            printers.emplace_back(std::move(dev), desc, std::move(conf), *alt, rwEp->first, rwEp->second);
        }
    }
    return printers;
}
