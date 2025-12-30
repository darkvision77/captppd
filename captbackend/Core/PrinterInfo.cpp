#include "PrinterInfo.hpp"
#include "Config.hpp"
#include <algorithm>
#include <initializer_list>
#include <ranges>
#include <string_view>
#include <cassert>

using namespace std::string_view_literals;

// Helper function for comparing URI component with URL decoding (%20 -> space)
template<typename Iter>
constexpr bool nextCmpDecoded(Iter& iter, Iter end, std::string_view target) noexcept {
    assert(target.size() != 0);
    for (char expected : target) {
        if (iter == end) {
            return false;
        }
        if (*iter == '%' && std::distance(iter, end) >= 3
            && *(iter + 1) == '2' && *(iter + 2) == '0') {
            // Decode %20 as space
            if (expected != ' ') {
                return false;
            }
            iter += 3;
        } else {
            if (*iter != expected) {
                return false;
            }
            ++iter;
        }
    }
    return true;
}

template<typename Iter>
constexpr bool nextCmp(Iter& iter, Iter end, std::initializer_list<std::string_view> targets) noexcept {
    for (std::string_view t : targets) {
        if (!nextCmpDecoded(iter, end, t)) {
            return false;
        }
    }
    return true;
}

// Helper function to URL-encode spaces as %20
std::ostream& urlEncodeSpaces(std::ostream& os, std::string_view str) {
    for (char c : str) {
        if (c == ' ') {
            os << "%20";
        } else {
            os << c;
        }
    }
    return os;
}

bool PrinterInfo::IsCaptPrinter() const noexcept {
    return this->CmdVersion.starts_with('1') && this->CommandSet == "CAPT";
}

std::ostream& PrinterInfo::WriteUri(std::ostream& os) const {
    if (os.good()) {
        // The URI must differ from the one issued by cups usb backend,
        // otherwise CUPS will not show our backend in the web UI.
        os << CAPTBACKEND_NAME "://" << this->Manufacturer << '/';
        urlEncodeSpaces(os, this->Model) << "?drv=capt&serial=" << this->Serial;
    }
    return os;
}

bool PrinterInfo::HasUri(std::string_view uri) const {
    static constexpr std::string_view proto = CAPTBACKEND_NAME "://";
    auto iter = uri.cbegin();
    auto end = uri.cend();
    if (!nextCmp(iter, end, {proto, this->Manufacturer, "/", this->Model, "?"})) {
        return false;
    }

    const std::string_view query(iter, end);
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
std::ostream& PrinterInfo::Report(std::ostream& stream) const {
    if (stream.good()) {
        // The (CAPTBACKEND_NAME) in the device-make-and-model is needed
        // so that backends can be distinguished in the CUPS web UI.
        stream << "direct "; // device-class
        this->WriteUri(stream) << ' '; // uri
        stream << '"' << this->Manufacturer << ' ' << this->Model << '"' << ' '; // device-make-and-model
        stream << '"' << this->Manufacturer << ' ' << this->Model << "(" CAPTBACKEND_NAME ")\" "; // device-info/name
        stream << '"' << this->DeviceId << '"'; // device-id
        stream << " \"\""; // device-location
    }
    return stream;
}
