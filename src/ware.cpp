#include "skopin/ware.hpp"

#include <stdexcept>

namespace skopin {

std::optional<std::string> validateWare(const Ware& ware) {
    if (ware.mark.empty()) {
        return std::string("empty mark");
    }
    for (char c : ware.mark) {
        if (c == ' ' || c == '\t' || c == '\n') {
            return std::string("bad mark");
        }
    }
    if (ware.height_cm <= 0 || ware.height_cm > kKilnHeightCm) {
        return std::string("bad height");
    }
    if (ware.thickness_mm <= 0) {
        return std::string("bad thickness");
    }
    return std::nullopt;
}

bool fitsInKiln(const std::vector<Ware>& stack, const Ware& next) {
    int total = next.height_cm + static_cast<int>(stack.size()) * kShelfCm;
    for (const Ware& w : stack) {
        total += w.height_cm;
    }
    return total <= kKilnHeightCm;
}

}  // namespace skopin
