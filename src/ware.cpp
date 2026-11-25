#include "skopin/ware.hpp"

#include <stdexcept>

namespace skopin {

std::optional<std::string> validateWare(const Ware& w){
     if (w.mark == "") {
        return "empty mark";
    }
    for (int i = 0; i < (int)w.mark.size(); i++) {
        if (w.mark[i] == ' ' || w.mark[i] == '\t' || w.mark[i] == '\n') {
            return "bad mark";
        }
    }
    if (w.height_cm <= 0 || w.height_cm > 48) {
        return "bad height";
    }
    if (w.thickness_mm <= 0) {
        return "bad thickness";
    }
    return std::nullopt;
}

bool fitsInKiln(const std::vector<Ware>& s, const Ware& w) {
    int sum = 0;
    for (int i = 0; i < (int)s.size(); i++) {
        sum = sum + s[i].height_cm + 2;
    }
    sum = sum + w.height_cm;
    if (sum <= 48) {
        return true;
    }
    return false;
}

}  // namespace skopin
