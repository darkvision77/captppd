#ifndef _CAPTBACKEND_CORE_PRINTER_INFO_HPP_
#define _CAPTBACKEND_CORE_PRINTER_INFO_HPP_

#include <ostream>
#include <string>
#include <string_view>

struct PrinterInfo {
    std::string DeviceId;
    std::string Manufacturer;
    std::string Model;
    std::string Description;
    std::string Serial;

    std::string CommandSet;
    std::string CmdVersion;

    static PrinterInfo Parse(std::string_view devId, std::string_view serial);

    bool IsCaptPrinter() const noexcept;
    std::string MakeUri() const;
    bool HasUri(std::string_view uri) const;

    void Report(std::ostream& stream) const;
};

#endif
