#include "PrinterInfo.hpp"
#include "Config.hpp"
#include <algorithm>
#include <ranges>
#include <sstream>
#include <string_view>

using namespace std::string_view_literals;

template<typename Iter1, typename Iter2>
static inline std::string makeString(Iter1 first, Iter2 last) {
    #if !defined(__llvm__) && defined(__GNUC__) && __GNUC__ < 12
    auto c = std::ranges::subrange(first, last) | std::views::common;
    return std::string(c.begin(), c.end());
    #else
    return std::string(first, last);
    #endif
}

template<typename Iter>
constexpr bool nextCmp(Iter& iter, std::string_view target) noexcept {
    const std::size_t size = target.size();
    const std::string_view str(iter, iter + size);
    iter += size;
    return str == target;
}

PrinterInfo PrinterInfo::Parse(std::string_view devId, std::string_view serial) {
    PrinterInfo info;
    info.DeviceId = devId;
    info.Serial = serial;
    for (const auto part : (devId | std::views::split(';'))) {
        auto delim = std::ranges::find(part, ':');
        if (delim == part.end()) {
            continue;
        }
        auto k = std::ranges::subrange(part.begin(), delim);
        std::string v = makeString(std::next(delim), part.end());
        if (std::ranges::equal(k, "MFG"sv) || std::ranges::equal(k, "MANUFACTURER"sv)) {
            info.Manufacturer = v;
        } else if (std::ranges::equal(k, "MDL"sv) || std::ranges::equal(k, "MODEL"sv)) {
            info.Model = v;
        } else if (std::ranges::equal(k, "DES"sv) || std::ranges::equal(k, "DESCRIPTION"sv)) {
            info.Description = v;
        } else if (std::ranges::equal(k, "CMD"sv) || std::ranges::equal(k, "COMMAND SET"sv)) {
            info.CommandSet = v;
        } else if (std::ranges::equal(k, "VER"sv)) {
            info.CmdVersion = v;
        }
    }
    return info;
}

bool PrinterInfo::IsCaptPrinter() const noexcept {
    return this->CmdVersion.starts_with('1') && this->CommandSet == "CAPT";
}

std::string PrinterInfo::MakeUri() const {
    std::ostringstream ss;
    // The URI must differ from the one issued by cups usb backend,
    // otherwise CUPS will not show our backend in the web UI.
    ss << CAPTBACKEND_NAME "://" << this->Manufacturer << '/' << this->Model << "?drv=capt&serial=" << this->Serial;
    return ss.str();
}

bool PrinterInfo::HasUri(std::string_view uri) const {
    constexpr std::string_view proto = CAPTBACKEND_NAME "://";
    const std::size_t minLen = proto.size() + this->Manufacturer.size() + this->Model.size() + 2;
    if (uri.size() < minLen || !uri.starts_with(proto)) {
        return false;
    }
    auto iter = uri.cbegin();
    if (!nextCmp(iter, proto)
        || !nextCmp(iter, this->Manufacturer)
        || *iter++ != '/'
        || !nextCmp(iter, this->Model)
        || *iter++ != '?') {
        return false;
    }

    const std::string_view query(iter, uri.cend());
    for (const auto part : (query | std::views::split('&'))) {
        auto delim = std::ranges::find(part, '=');
        if (delim == part.end()) {
            continue;
        }
        auto k = std::ranges::subrange(part.begin(), delim);
        auto v = std::ranges::subrange(std::next(delim), part.end());
        if (std::ranges::equal(k, "serial"sv) && std::ranges::equal(v, this->Serial)) {
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
