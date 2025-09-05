#include "PrinterInfo.hpp"
#include <ranges>
#include <sstream>
#include <string_view>

static std::pair<std::string_view, std::string_view> splitKv(std::string_view str, char sep) noexcept {
    std::size_t delimPos = str.find(sep);
    return {str.substr(0, delimPos), str.substr(delimPos + 1)};
}

PrinterInfo PrinterInfo::Fetch(UsbPrinter& dev) {
    PrinterInfo info;
    info.DeviceId = dev.GetDeviceId();
    info.Serial = dev.GetSerial();
    for (const auto line : (info.DeviceId | std::views::split(';'))) {
        #if defined(__GNUC__) && __GNUC__ < 12
            auto c = line | std::views::common;
            std::string str(c.begin(), c.end());
        #else
            std::string_view str(line);
        #endif
        auto [k, v] = splitKv(str, ':');
        if (k == "MFG" || k == "MANUFACTURER") {
            info.Manufacturer = v;
        } else if (k == "MDL" || k == "MODEL") {
            info.Model = v;
        } else if (k == "DES" || k == "DESCRIPTION") {
            info.Description = v;
        } else if (k == "CMD" || k == "COMMAND SET") {
            info.CommandSet = v;
        } else if (k == "VER") {
            info.CmdVersion = v;
        }
    }
    return info;
}

bool PrinterInfo::IsCaptPrinter() const noexcept {
    return this->CommandSet == "CAPT" && this->CmdVersion.starts_with('1');
}

std::string PrinterInfo::MakeUri() const {
    std::ostringstream ss;
    // The URI must differ from the one issued by cups usb backend,
    // otherwise CUPS will not show our backend in the web UI.
    ss << CAPTBACKEND_NAME "://" << this->Manufacturer << '/' << this->Model << "?drv=capt&serial=" << this->Serial;
    return ss.str();
}

bool PrinterInfo::HasUri(std::string_view uri) const {
    std::ostringstream ss;
    ss << CAPTBACKEND_NAME "://" << this->Manufacturer << '/' << this->Model << "?";
    const std::string_view prefix = ss.view();
    if (!uri.starts_with(prefix)) {
        return false;
    }
    for (const auto line : (uri.substr(prefix.size()) | std::views::split('&'))) {
        #if defined(__GNUC__) && __GNUC__ < 12
            auto c = line | std::views::common;
            std::string str(c.begin(), c.end());
        #else
            std::string_view str(line);
        #endif
        auto [k, v] = splitKv(str, '=');
        if (k == "serial" && v == this->Serial) {
            return true;
        }
    }
    return false;
}

// device-class
// uri
// device-make-and-model
// device-info
// device-id
// device-location
void PrinterInfo::Report(std::ostream& stream) const {
    // The (CAPTBACKEND_NAME) in the device-make-and-model is needed
    // so that backends can be distinguished in the CUPS web UI.
    stream << "direct "; // device-class
    stream << this->MakeUri() << ' '; // uri
    stream << '"' << this->Manufacturer << ' ' << this->Model << " (" CAPTBACKEND_NAME ")\" "; // device-make-and-model/description
    stream << '"' << this->Manufacturer << ' ' << this->Model << '"' << ' '; // device-info/name
    stream << '"' << this->DeviceId << '"'; // device-id
    stream << " \"\""; // device-location
    stream << std::endl;
}
