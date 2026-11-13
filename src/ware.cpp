#include "skopin/ware.hpp"

#include <optional>
#include <string>
#include <vector>

namespace skopin {

std::optional<std::string> validateWare(const Ware& ware) {
    if (ware.mark.empty()) {
        return "empty mark";
    }

    for (char ch : ware.mark) {
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

    return std::nullopt;
}

bool fitsInKiln(const std::vector<Ware>& stack, const Ware& next) {
    int total = 0;
    for (const Ware& ware : stack) {
        total += ware.height_cm;
    }
    total += static_cast<int>(stack.size()) * kShelfCm;
    total += next.height_cm;
    return total <= kKilnHeightCm;
}

}