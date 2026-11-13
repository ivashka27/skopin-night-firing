
#include "skopin/ware.hpp"

namespace skopin {

std::optional<std::string> validateWare(const Ware& ware) {
    if (ware.mark.empty()) {
        return "empty mark";
    }

    if (ware.mark.find_first_of(" \t\n\r\f\v") != std::string::npos) {
        return "bad mark";
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
    long long totalHeight = next.height_cm;

    for (const Ware& ware : stack) {
        totalHeight += ware.height_cm;
    }

    totalHeight += static_cast<long long>(stack.size()) * kShelfCm;

    return totalHeight <= kKilnHeightCm;
}

}  // namespace skopin