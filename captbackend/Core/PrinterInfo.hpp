#ifndef _CAPTBACKEND_CORE_PRINTER_INFO_HPP_
#define _CAPTBACKEND_CORE_PRINTER_INFO_HPP_

#include <ostream>
#include <string>
#include <string_view>
#include <ranges>
#include <algorithm>

namespace impl {
    template<typename Iter1, typename Iter2>
    constexpr std::string makeString(Iter1 first, Iter2 last) {
        #if !defined(__llvm__) && defined(__GNUC__) && __GNUC__ < 12
        auto c = std::ranges::subrange(first, last) | std::views::common;
        return std::string(c.begin(), c.end());
        #else
        return std::string(first, last);
        #endif
    }
}

struct PrinterInfo {
    std::string DeviceId;
    std::string Manufacturer;
    std::string Model;
    std::string Description;
    std::string Serial;

    std::string CommandSet;
    std::string CmdVersion;

    template<typename String1, typename String2>
    [[nodiscard]] static constexpr PrinterInfo Parse(String1&& devId, String2&& serial) {
        using namespace std::string_view_literals;
        PrinterInfo info;
        info.DeviceId = std::forward<String1>(devId);
        info.Serial = std::forward<String2>(serial);
        for (const auto part : (info.DeviceId | std::views::split(';'))) {
            auto delim = std::ranges::find(part, ':');
            if (delim == part.end()) {
                continue;
            }
            auto k = std::ranges::subrange(part.begin(), delim);
            std::string v = impl::makeString(std::next(delim), part.end());
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

    [[nodiscard]] bool IsCaptPrinter() const noexcept;
    std::ostream& WriteUri(std::ostream& os) const;
    [[nodiscard]] bool HasUri(std::string_view uri) const;

    void Report(std::ostream& stream) const;
};

#endif
