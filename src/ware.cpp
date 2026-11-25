
#include "skopin/ware.hpp"

#include <stdexcept>

namespace skopin {

std::optional<std::string> validateWare(const Ware& ware) {
    if (ware.mark.empty()) {
        return "empty mark";
    }

    bool bad = false;

    for (int i = 0; i < ware.mark.size(); i++) {
        if (ware.mark[i] == ' ' || ware.mark[i] == '\t' || ware.mark[i] == '\n') {
            bad = true;
        }
    }

    if (bad) {
        return "bad mark";
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
    int sum = next.height_cm;
    int count = 0;

    for (const Ware& ware : stack) {
        sum += ware.height_cm;
        count++;
    }

    sum += count * 2;

    return sum <= kKilnHeightCm;
}
}  // namespace skopin