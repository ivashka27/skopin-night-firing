#include "skopin/ware.hpp"
#include <string>
#include <vector>
#include <stdexcept>
#include "skopin/measures.hpp"

namespace skopin {

std::optional<std::string> validateWare(const Ware& ware) {
    if (ware.mark.empty() == true) {
        return "empty mark";
    }
    for (int i = 0; i < ware.mark.size(); i++) {
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
    else {
        return std::nullopt;
    }
}

bool fitsInKiln(const std::vector<Ware>& stack, const Ware& next) {
    int total = 0;
    for (int i = 0; i < stack.size(); i++) {
        total = total + stack[i].height_cm;
    }
    total = total + next.height_cm;
    total = total + kShelfCm * stack.size();
    return total <= kKilnHeightCm;
}

}  // namespace skopin
