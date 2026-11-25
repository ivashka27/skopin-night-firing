#include "skopin/ware.hpp"

#include <stdexcept>

namespace skopin {

std::optional<std::string> validateWare(const Ware& ware) {
    if (ware.mark.empty()) {
        return "empty mark";
    }
    for (char symbol : ware.mark){
        if (symbol == ' ' || symbol == '\t' || symbol == '\n' ){
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
    long long total_hight = next.height_cm;

    for (const Ware& ware : stack) {
        total_hight = total_hight + ware.height_cm;
    }
    total_hight = total_hight + stack.size() * kShelfCm;

    if (total_hight <= kKilnHeightCm) {
        return true;
    }
    else {
        return false;
    }
}

}  // namespace skopin
