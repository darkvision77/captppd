#include "Core/RasterError.hpp"
#include "Core/StateReporter.hpp"
#include "Core/CaptPrinter.hpp"
#include "Core/Log.hpp"
#include "Core/PrinterInfo.hpp"
#include "Core/StopToken.hpp"
#include "Cups/CupsRasterStreambuf.hpp"
#include "UsbBackend/UsbBackend.hpp"
#include "UsbBackend/UsbError.hpp"
#include "UsbBackend/UsbPrinter.hpp"
#include "UsbBackend/UsbStreambuf.hpp"
#include "Config.hpp"
#include <cassert>
#include <csignal>
#include <exception>
#include <iostream>
#include <optional>
#include <cups/backend.h>
#include <libcapt/UnexpectedBehaviourError.hpp>
#include <libcapt/Version.hpp>
#include <string_view>
#include <thread>
#include <vector>

using namespace std::literals::chrono_literals;

static StopSource stopSource;

static void sighandler([[maybe_unused]] int sig) noexcept {
    stopSource.request_stop();
}

static inline std::optional<std::string_view> getEnv(const char* key) noexcept {
    const char* val = std::getenv(key);
    return val == nullptr ? std::nullopt : std::optional(std::string_view(val));
}

static std::optional<UsbPrinter> connectByUri(StopToken stopToken, UsbBackend& backend, std::string_view uri) {
    while (!stopToken.stop_requested()) {
        std::vector<UsbPrinter> printers = backend.GetPrinters();
        for (UsbPrinter& p : printers) {
            auto info = p.GetPrinterInfo();
            if (!info || !info->IsCaptPrinter()) {
                continue;
            }
            if (info->HasUri(uri)) {
                return std::move(p);
            }
        }
        Log::Info() << "Waiting for printer to become available";
        std::this_thread::sleep_for(5s);
    }
    return std::nullopt;
}

static void discover(UsbBackend& backend) {
    std::vector<UsbPrinter> printers = backend.GetPrinters();
    Log::Debug() << "Discovered " << printers.size() << " printer devices";
    for (UsbPrinter& p : printers) {
        auto info = p.GetPrinterInfo();
        if (!info) {
            continue;
        }
        if (!info->IsCaptPrinter()) {
            Log::Debug() << "Skipping non-CAPT v1 printer (" << info->DeviceId << ')';
            continue;
        }
        info->Report(std::cout);
    }
}

int main(int argc, const char* argv[]) {
    std::signal(SIGPIPE, SIG_IGN);
    std::signal(SIGTERM, sighandler);
    std::signal(SIGINT, sighandler);
    StopToken stopToken = stopSource.get_token();

    if (argc != 1 && argc != 6 && argc != 7) {
        std::cout << "Usage: " << argv[0] << " job-id user title copies options [file]" << std::endl;
        return CUPS_BACKEND_FAILED;
    }

    Log::Debug() << CAPTBACKEND_NAME " version " CAPTBACKEND_VERSION_STRING;
    Log::Debug() << "libcapt version " LIBCAPT_VERSION_STRING;

    try {
        StateReporter reporter(std::cerr);
        UsbBackend backend;
        backend.Init();

        if (argc == 1) {
            discover(backend);
            return CUPS_BACKEND_OK;
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

        reporter.SetReason("connecting-to-device", true);
        std::optional<UsbPrinter> targetPrinter = connectByUri(stopToken, backend, *targetUri);
        reporter.SetReason("connecting-to-device", false);
        if (stopToken.stop_requested()) {
            return CUPS_BACKEND_OK;
        }
        if (!targetPrinter) {
            Log::Critical() << "Device not found";
            return CUPS_BACKEND_FAILED;
        }
        targetPrinter->Open();
        Log::Debug() << "Device opened";

        UsbStreambuf streambuf(*targetPrinter);
        std::iostream printerStream(&streambuf);
        printerStream.exceptions(std::ios_base::failbit | std::ios_base::badbit);

        CaptPrinter printer(printerStream, reporter);
        printer.ReserveUnit();
        Log::Info() << "Unit reserved";

        bool success;
        if (*contentType == "application/vnd.cups-command") {
            success = printer.Clean(stopToken);
        } else {
            assert(*contentType == "application/vnd.cups-raster");
            CupsRasterStreambuf cupsRaster;
            if (!cupsRaster.Open(argc == 7 ? argv[6] : nullptr)) {
                Log::Critical() << "Failed to open raster stream";
                return CUPS_BACKEND_FAILED;
            }
            success = printer.Print(stopToken, cupsRaster);
        }

        Log::Debug() << "Releasing unit...";
        printer.GoOffline();
        printer.ReleaseUnit();
        Log::Debug() << "Unit released";
        return success ? CUPS_BACKEND_OK : CUPS_BACKEND_FAILED;
    } catch (const Capt::UnexpectedBehaviourError& e) {
        Log::Critical() << "Protocol fault: " << e.what();
    } catch (const UsbError& e) {
        Log::Critical() << "USB backend error: " << e.what() << " (" << e.StrErrcode() << ')';
    } catch (const RasterError& e) {
        Log::Critical() << "Raster error: " << e.what();
    } catch (const std::exception& e) {
        Log::Critical() << "Unhandled exception: " << e.what();
    }
    return CUPS_BACKEND_FAILED;
}
