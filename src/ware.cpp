#include "skopin/ware.hpp"

namespace skopin {

std::optional<std::string> validateWare(const Ware& ware) {
    if (ware.mark.empty()) {
        return "empty mark";
    }
    for (std::size_t i = 0; i < ware.mark.size(); i++) {
        char ch = ware.mark[i];
        if (ch == ' ' || ch == '\t' || ch == '\n') {
            return "bad mark";
        }
    }
    if (ware.height_cm <= 0 || ware.height_cm > kKilnHeightCm) {
        return "bad height";
    }
    if (ware.thickness_mm <= 0) {
        return "bad thickness";
    }
    return {};
}

bool fitsInKiln(const std::vector<Ware>& stack, const Ware& next) {
    int h = 0;
    for (std::size_t i = 0; i < stack.size(); i++) {
        h += stack[i].height_cm;
        h += kShelfCm;
    }
    h += next.height_cm;
    if (h <= kKilnHeightCm) {
        return true;
    }
    return false;
}

}  // namespace skopin