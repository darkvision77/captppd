#include "PrinterInfo.hpp"
#include "Config.hpp"
#include <algorithm>
#include <ranges>
#include <string_view>

using namespace std::string_view_literals;

template<typename Iter>
constexpr bool nextCmp(Iter& iter, std::string_view target) noexcept {
    const std::size_t size = target.size();
    const std::string_view str(iter, iter + size);
    iter += size;
    return str == target;
}

// Helper function for comparing URI component with URL decoding (%20 -> space)
template<typename Iter>
bool nextCmpDecoded(Iter& iter, Iter end, std::string_view target) noexcept {
    for (char expected : target) {
        if (iter == end) {
            return false;
        }
        if (*iter == '%' && std::distance(iter, end) >= 3 &&
            *(iter + 1) == '2' && *(iter + 2) == '0') {
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

// Helper function to URL-encode spaces as %20
std::string urlEncodeSpaces(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    for (char c : str) {
        if (c == ' ') {
            result += "%20";
        } else {
            result += c;
        }
    }
    return result;
}

bool PrinterInfo::IsCaptPrinter() const noexcept {
    return this->CmdVersion.starts_with('1') && this->CommandSet == "CAPT";
}

std::ostream& PrinterInfo::WriteUri(std::ostream& os) const {
    if (os.good()) {
        // The URI must differ from the one issued by cups usb backend,
        // otherwise CUPS will not show our backend in the web UI.
        os << CAPTBACKEND_NAME "://" << this->Manufacturer << '/' << urlEncodeSpaces(this->Model) << "?drv=capt&serial=" << this->Serial;
    }
    return os;
}

bool PrinterInfo::HasUri(std::string_view uri) const {
    constexpr std::string_view proto = CAPTBACKEND_NAME "://";
    const std::size_t minLen = proto.size() + this->Manufacturer.size() + this->Model.size() + 2;
    if (uri.size() < minLen || !uri.starts_with(proto)) {
        return false;
    }
    auto iter = uri.cbegin();
    auto end = uri.cend();
    if (!nextCmp(iter, proto)
        || !nextCmp(iter, this->Manufacturer)
        || *iter++ != '/'
        || !nextCmpDecoded(iter, end, this->Model)
        || iter == end || *iter++ != '?') {
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
    this->WriteUri(stream) << ' '; // uri
    stream << '"' << this->Manufacturer << ' ' << this->Model << '"' << ' '; // device-make-and-model
    stream << '"' << this->Manufacturer << ' ' << this->Model << "(" CAPTBACKEND_NAME ")\" "; // device-info/name
    stream << '"' << this->DeviceId << '"'; // device-id
    stream << " \"\""; // device-location
    stream << std::endl;
}
