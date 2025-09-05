#ifndef _CAPTBACKEND_PRINTER_INFO_HPP_
#define _CAPTBACKEND_PRINTER_INFO_HPP_

#include "UsbBackend/UsbPrinter.hpp"
#include <ostream>
#include <string>

struct PrinterInfo {
    std::string DeviceId;
    std::string Manufacturer;
    std::string Model;
    std::string Description;
    std::string Serial;

    std::string CommandSet;
    std::string CmdVersion;

    static PrinterInfo Fetch(UsbPrinter& dev);

    bool IsCaptPrinter() const noexcept;
    std::string MakeUri() const;
    bool HasUri(std::string_view uri) const;

    void Report(std::ostream& stream) const;
};

#endif
