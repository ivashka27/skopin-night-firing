#include "skopin/ware.hpp"


#include <optional>
#include <string>
#include <vector>

namespace skopin {

std::optional<std::string> validateWare(const Ware& ware) {
    if (ware.mark.empty()) {
        return "empty mark";
    }

    if (ware.mark.find_first_of(" \t\n\r") != std::string::npos) {
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
    int totalHeight = next.height_cm;

    for (const Ware& ware : stack) {
        totalHeight += ware.height_cm + kShelfCm;
    }

    return totalHeight <= kKilnHeightCm;
}

} 
