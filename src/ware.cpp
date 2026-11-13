#include "skopin/ware.hpp"

#include <stdexcept>

namespace skopin {

std::optional<std::string> validateWare(const Ware& w) {
    if (w.mark.empty()){
        return "empty mark";
    }
    if ((w.mark.find(" ") != std::string::npos) ||
        ((w.mark.find("\t")) != std::string::npos) ||
        (w.mark.find("\n") != std::string::npos)){
        return "bad mark";
    }
    if (w.height_cm <= 0 || w.height_cm > 48){
        return "bad height";
    }
    if (w.thickness_mm <= 0){
        return "bad thickness";
    }
    return std::nullopt;
}

bool fitsInKiln(const std::vector<Ware>& stack, const Ware& next) {
    int all_height = 0;
    for (size_t i = 0; i < stack.size(); i++){
        const Ware& w = stack[i];
        all_height += w.height_cm;
        all_height += 2;
    }
    all_height += next.height_cm;
    return (all_height <= 48);
}

}  // namespace skopin
