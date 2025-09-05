#include "Core/StateReporter.hpp"
#include "Core/CaptPrinter.hpp"
#include "Core/Log.hpp"
#include "Cmds.hpp"
#include "PrinterInfo.hpp"
#include "UsbBackend/UsbBackend.hpp"
#include "UsbBackend/UsbError.hpp"
#include "UsbBackend/UsbPrinter.hpp"
#include "UsbBackend/UsbStreambuf.hpp"
#include <cassert>
#include <csignal>
#include <cstdio>
#include <exception>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <stop_token>
#include <cups/backend.h>
#include <libcapt/UnexpectedBehaviourError.hpp>
#include <string_view>
#include <vector>
#include <unistd.h>

static std::stop_source stopSource;

static void sighandler([[maybe_unused]] int sig) noexcept {
    stopSource.request_stop();
}

static std::optional<std::string_view> getEnv(const std::string& key) noexcept {
    const char* val = std::getenv(key.c_str());
    return val == nullptr ? std::nullopt : std::optional(std::string_view(val));
}

static std::optional<PrinterInfo> getPrinterInfo(UsbPrinter& p) {
    if (!p.Open()) {
        Log::Debug() << "Failed to open device " << std::hex << std::setfill('0')
            << std::setw(4) << p.VendorId() << ':' << std::setw(4) << p.VendorId() << ", skipping";
        return std::nullopt;
    }
    PrinterInfo info = PrinterInfo::Fetch(p);
    p.Close();
    return info;
}

static UsbPrinter* findPrinterByUri(std::vector<UsbPrinter>& printers, std::string_view uri) {
    for (UsbPrinter& p : printers) {
        auto info = getPrinterInfo(p);
        if (!info || !info->IsCaptPrinter()) {
            continue;
        }
        if (info->HasUri(uri)) {
            return &p;
        }
    }
    return nullptr;
}

static int reportDevices(std::vector<UsbPrinter>& printers) {
    for (UsbPrinter& p : printers) {
        auto info = getPrinterInfo(p);
        if (!info) {
            continue;
        }
        if (!info->IsCaptPrinter()) {
            Log::Debug() << "Skipping non-CAPT v1 printer (" << info->DeviceId << ')';
            continue;
        }
        info->Report(std::cout);
    }
    return CUPS_BACKEND_OK;
}

int main(int argc, const char* argv[]) {
    std::signal(SIGPIPE, SIG_IGN);
    std::signal(SIGTERM, sighandler);
    std::signal(SIGINT, sighandler);

    if (argc != 1 && argc != 6 && argc != 7) {
        std::cout << "Usage: " << argv[0] << " job-id user title copies options [file]" << std::endl;
        return CUPS_BACKEND_FAILED;
    }

    Log::Debug() << CAPTBACKEND_NAME " version " CAPTBACKEND_VERSION;
    Log::Debug() << "libcapt version " LIBCAPT_VERSION;

    UsbBackend backend;
    if (!backend.Init()) {
        Log::Critical() << "Failed to init usb backend";
        return CUPS_BACKEND_FAILED;
    }

    std::vector<UsbPrinter> printers = backend.GetPrinters();
    Log::Debug() << "Discovered " << printers.size() << " printer devices";

    if (argc == 1) {
        return reportDevices(printers);
    }

    auto targetUri = getEnv("DEVICE_URI");
    if (!targetUri) {
        Log::Critical() << "Failed to get target device uri";
        return CUPS_BACKEND_FAILED;
    }
    auto contentType = getEnv("FINAL_CONTENT_TYPE");
    if (!contentType) {
        Log::Critical() << "Content type is not defined";
        return CUPS_BACKEND_FAILED;
    }
    if (*contentType != "application/vnd.cups-raster" && *contentType != "application/vnd.cups-command") {
        contentType = getEnv("CONTENT_TYPE");
        if (!contentType || *contentType != "application/vnd.cups-command") {
            Log::Critical() << "Unsupported content type";
            return CUPS_BACKEND_FAILED;
        }
    }

    UsbPrinter* targetPrinter = findPrinterByUri(printers, *targetUri);
    if (targetPrinter == nullptr) {
        Log::Critical() << "Printer not found";
        return CUPS_BACKEND_FAILED;
    }
    if (!targetPrinter->Open()) {
        Log::Critical() << "Failed to open target device";
        return CUPS_BACKEND_FAILED;
    }
    if (!targetPrinter->DetachKernelDriver()) {
        Log::Critical() << "Failed to detach kernel driver";
        return CUPS_BACKEND_FAILED;
    }
    if (!targetPrinter->Claim()) {
        Log::Critical() << "Failed to claim interface";
        return CUPS_BACKEND_FAILED;
    }
    targetPrinter->Reset();
    Log::Debug() << "Device opened";

    UsbStreambuf streambuf(*targetPrinter);
    std::iostream printerStream(&streambuf);
    StateReporter reporter(std::cerr);

    std::stop_token stopToken = stopSource.get_token();
    int ret = CUPS_BACKEND_OK;
    try {
        printerStream.exceptions(std::ios_base::failbit | std::ios_base::badbit);
        CaptPrinter printer(printerStream, reporter);
        printer.ReserveUnit();
        Log::Info() << "Unit reserved";

        if (*contentType == "application/vnd.cups-raster") {
            int fd = STDIN_FILENO;
            std::unique_ptr<FILE, decltype(&fclose)> filePtr(nullptr, fclose);
            if (argc == 7) {
                FILE* file = fopen(argv[6], "rb");
                if (file == nullptr) {
                    Log::Critical() << "Failed to open input file";
                    return CUPS_BACKEND_FAILED;
                }
                filePtr.reset(file);
                fd = fileno(file);
            }
            ret = CmdPrint(stopToken, reporter, printer, fd);
        } else {
            ret = CmdClean(stopToken, printer);
        }

        printer.GoOffline();
        printer.ReleaseUnit();
        Log::Info() << "Unit released";
    } catch (const Capt::UnexpectedBehaviourError& e) {
        Log::Critical() << "Protocol fault (" << e.what() << ')';
        return CUPS_BACKEND_FAILED;
    } catch (const UsbError& e) {
        Log::Critical() << "USB backend error (" << e.what() << ')';
        return CUPS_BACKEND_FAILED;
    } catch (const std::exception& e) {
        Log::Critical() << "Unhandled exception (" << e.what() << ')';
        return CUPS_BACKEND_FAILED;
    }
    return ret;
}
