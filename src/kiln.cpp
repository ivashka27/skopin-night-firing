#include "skopin/kiln.hpp"

#include <stdexcept>

namespace skopin {

Kiln::Kiln() {
    cone_ = Cone::Cold;
    door_ = Door::Closed;
    stokes_ = 0;
    stack_.clear();
    glaze_.clear();
}

Cone Kiln::cone() const {
    return cone_;
}

Door Kiln::door() const {
    return door_;
}

int Kiln::stokes() const {
    return stokes_;
}

std::vector<Ware> Kiln::stack() const {
    return stack_;
}

void Kiln::open() {
    if (door_ == Door::Open) {
        throw KilnError("Door already open");
    }

    if (cone_ == Cone::Glaze) {
        throw KilnError("Cannot open door at glaze temperature");
    }

    door_ = Door::Open;
}

void Kiln::close() {
    if (door_ == Door::Closed) {
        throw KilnError("Door already closed");
    }

    door_ = Door::Closed;
}

void Kiln::fire() {
    if (door_ != Door::Closed || cone_ == Cone::Glaze) {
        throw KilnError("Cannot fire in ware");
    }

    for (const Ware& ware : stack_) {
        if (ware.thickness_mm > kCrackThicknessMm) {
            throw KilnError("Ware too thick");
        }
    }

    if (cone_ == Cone::Bisque) {
        for (const Ware& ware : stack_) {
            if (ware.program == Program::Bisque) {
                throw KilnError("Bisque ware cannot reach glaze temperature");
            }
        }

        for (std::size_t i = 0; i < stack_.size(); ++i) {
            if (stack_[i].program == Program::Glaze) {
                glaze_[i] = true;
            }
        }

        cone_ = Cone::Glaze;
    } else {
        cone_ = Cone::Bisque;
    }

    ++stokes_;
}

void Kiln::cool() {
if (door_ != Door::Closed) {
        throw KilnError("Cannot cool with open door");
    }

    if (cone_ == Cone::Cold) {
        throw KilnError("Kiln already cold");
    }

    if (cone_ == Cone::Bisque) {
        for (std::size_t i = 0; i < stack_.size(); ++i) {
            if (stack_[i].program == Program::Glaze && !glaze_[i]) {
                throw KilnError("Glaze ware has't reached glaze temperature");
            }
        }

        cone_ = Cone::Cold;
    } else {
        cone_ = Cone::Bisque;
    }

    ++stokes_;
}

void Kiln::load(const Ware&ware) {
if (door_ != Door::Open) {
        throw KilnError("Door closed");
    }

    if (cone_ == Cone::Glaze) {
        throw KilnError("Cannot load at glaze temperature");
    }

    if (cone_ == Cone::Bisque && ware.program != Program::Glaze) {
        throw KilnError("Glaze ware can be loaded only bisque temperature");
    }

    if (validateWare(ware).has_value()) {
        throw KilnError("Invalid ware");
    }

    for (const Ware& item : stack_) {
        if (item.mark == ware.mark) {
            throw KilnError("Duplicate mark");
        }

        if (item.kind == WareKind::Sagger || ware.kind == WareKind::Sagger) {
            throw KilnError("Sagger must be in kiln alone");
        }
    }

    if (ware.kind == WareKind::Sagger && !stack_.empty()) {
        throw KilnError("Sagger must be in kiln alone");
    }

    if (!fitsInKiln(stack_, ware)) {
        throw KilnError("Ware does't fit");
    }

    stack_.push_back(ware);
    glaze_.push_back(false);
}

void Kiln::unload(const std::string&mark) {
if (door_ != Door::Open) {
        throw KilnError("Door closed");
    }

    if (stack_.empty()) {
        throw KilnError("Kiln empty");
    }

    if (stack_.back().mark != mark) {
        throw KilnError("Ware not on top");
    }

    if (stack_.back().program == Program::Glaze && cone_ != Cone::Cold) {
        throw KilnError("Glaze ware can be unloaded only from a cold kiln");
    }

    stack_.pop_back();
    glaze_.pop_back();
}

}  // namespace skopin
