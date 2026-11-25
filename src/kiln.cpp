#include "skopin/kiln.hpp"

#include <stdexcept>

namespace skopin {

Kiln::Kiln() {
    
}

Cone Kiln::cone() const {
    return _coneConditional;
}

Door Kiln::door() const {
    return _doorConditional;
}

int Kiln::stokes() const {
    return _stokes;
}

std::vector<Ware> Kiln::stack() const {
    return _stack;
}
    

void Kiln::open() {

    if (_doorConditional == Door::Open) {
        throw KilnError("The door have already opened");
    }
    if (_coneConditional == Cone::Glaze) {
        throw KilnError("Very hot");
    }

    _doorConditional = Door::Open;
}

void Kiln::close() {
    if (_doorConditional == Door::Closed) {
        throw KilnError("The door have already closed");
    }

    _doorConditional = Door::Closed;
}

void Kiln::fire() {
    if(_doorConditional != Door::Closed)
    {
        throw KilnError("The door is open!");
    }
    if (_coneConditional == Cone::Glaze) {
        throw KilnError("Max temperature now");
    }

    for (auto agr : _stack) {
        if (agr.thickness_mm > kCrackThicknessMm) {
            throw KilnError("Fat");
        }
    }

    if (_coneConditional == Cone::Bisque) {
        for (auto agr : _stack) {
            if (agr.program == Program::Bisque) {
                throw KilnError("There is clay inside");
            }
        }

        _coneConditional = Cone::Glaze;

        for (int i = 0; i < _deified.size(); ++i) {
            _deified[i] = true;
        }
    }
    else {
        _coneConditional = Cone::Bisque;
    }

    _stokes++;
}

void Kiln::cool() {
    if (_doorConditional != Door::Closed) {
        throw KilnError("The door is opened");
    }
    if (_coneConditional == Cone::Cold) {
        throw KilnError("Already cold");
    }

    if (_coneConditional == Cone::Bisque) {
        for (int i = 0; i < _stack.size(); i++) {
            if (_stack[i].program == Program::Glaze && !_deified[i]) {
                throw KilnError("Glaze ware has not seen glaze cone");
            }
        }

        _coneConditional = Cone::Cold;
    }
    else if (_coneConditional == Cone::Glaze) {
        _coneConditional = Cone::Bisque;
    }

    _stokes++;
}

void Kiln::load(const Ware& ware) {
    if (_doorConditional != Door::Open) {
        throw KilnError("The door is closed");
    }

    if (_coneConditional == Cone::Glaze ||
        (_coneConditional == Cone::Bisque && ware.program == Program::Bisque)) {
        throw KilnError("Unacceptable temperature");
    }

    if (validateWare(ware) != std::nullopt) {
        throw KilnError("Invalid ware");
    }

    for (auto agr : _stack) {
        if (agr.mark == ware.mark) {
            throw KilnError("Duplicate mark");
        }
    }

    if (ware.kind == WareKind::Sagger && !_stack.empty()) {
        throw KilnError("Sagger must be loaded into empty kiln");
    }
    for (const auto& agr : _stack) {
        if (agr.kind == WareKind::Sagger) {
            throw KilnError("Cannot load ware with sagger already inside");
        }
    }
    if (!fitsInKiln(_stack, ware)) {
        throw KilnError("Ware does not fit by height");
    }
    _stack.push_back(ware);
    _deified.push_back(false);
}

void Kiln::unload(const std::string& mark) {
    if (_doorConditional != Door::Open) {
        throw KilnError("The door is closed");
    }
    if (_stack.empty()) {
        throw KilnError("Kiln is empty");
    }
    if (_stack.back().mark != mark) {
        throw KilnError("Mark does not match");
    }
    if (_stack.back().program == Program::Glaze && _coneConditional != Cone::Cold) {
        throw KilnError("The ware are not cold");
    }
    _stack.pop_back();
    _deified.pop_back();
}

}  // namespace skopin
