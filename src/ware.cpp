#include "skopin/ware.hpp"

#include <stdexcept>

namespace skopin {

std::optional<std::string> validateWare(const Ware& w) {
    if (w.mark.empty()) {
        return "empty mark";
    }
    for (char c : w.mark) {
        if (c == ' ' || c == '\t' || c == '\n') {
            return "bad mark";
        }
    }
    if (w.height_cm <= 0 || w.height_cm > kKilnHeightCm) {
        return "bad height";
    }
    if (w.thickness_mm <= 0) {
        return "bad thickness";
    }
    return std::nullopt;
}


bool fitsInKiln(const std::vector<Ware>& stack, const Ware& next) {
    int vse = next.height_cm;

    for (const Ware& item : stack) {
        vse = vse + item.height_cm + kShelfCm;
    }

    return vse <= kKilnHeightCm;
}


}  // namespace skopin
