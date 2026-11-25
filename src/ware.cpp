#include "skopin/ware.hpp"

#include <stdexcept>

namespace skopin {

std::optional<std::string> validateWare(const Ware& ware) {
    if (ware.mark.empty()) {
        return "empty mark";
    }
    for (auto c : ware.mark) {
        if (c == ' ' || c == '\n' || c == '\t' || c == '\r') {
            return "bad mark";
        }
    }
    if (ware.height_cm <= 0 || ware.height_cm > 48) {
        return "bad height";
    }
    if (ware.thickness_mm <= 0) {
        return "bad thickness";
    }
    return std::nullopt;
}

bool fitsInKiln(const std::vector<Ware>& stack, const Ware& next) {
    int height = 0;
    for (auto agr : stack) {
        height += agr.height_cm;
    }
    height += stack.size() * 2;
    height += next.height_cm;

    if (height > 48) {
        return false;
    }

    return true;
}

}  // namespace skopin
