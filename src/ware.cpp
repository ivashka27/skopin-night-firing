#include "skopin/ware.hpp"

#include <stdexcept>

namespace skopin {

std::optional<std::string> validateWare(const Ware& ware) {
    if (ware.mark == ""){
        return "empty mark";
    }
    if (ware.mark.find(" ") != std::string::npos || ware.mark.find("\t") != std::string::npos || ware.mark.find("\n") != std::string::npos){
        return "bad mark";
    }
    if (ware.height_cm <= 0 || ware.height_cm > kKilnHeightCm){
        return "bad height";
    }
    if (ware.thickness_mm <= 0){
        return "bad thickness";
    }
    return std::nullopt;
}

bool fitsInKiln(const std::vector<Ware>& stack, const Ware& next) {
    int sm = 0;
    for (const Ware& ware : stack){
        sm += ware.height_cm;
    }
    sm += kShelfCm * stack.size();
    if (sm + next.height_cm<= kKilnHeightCm){
        return true;
    }
    return false;
}

}  // namespace skopin
