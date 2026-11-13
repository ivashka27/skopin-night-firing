#include "skopin/ware.hpp"

namespace skopin {

std::optional<std::string> validateWare(const Ware& ware) {
    if (ware.mark == "") {
        return "empty mark";
    }

    int n = ware.mark.size();
    for (int i = 0; i < n; i++) {
        char c = ware.mark[i];
        if (c == ' ' || c == '\t' || c == '\n') {
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
    long long height = next.height_cm;
    for (std::size_t i = 0; i < stack.size(); i++) {
        height = height + stack[i].height_cm + kShelfCm;
    }
    return height <= kKilnHeightCm;
}

}  // namespace skopin
