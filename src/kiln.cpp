#include "skopin/kiln.hpp"

namespace skopin {

Kiln::Kiln()
    : cone_(Cone::Cold),
      door_(Door::Closed),
      stokes_(0) {
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
        throw KilnError("Door is already open");
    }

    if (cone_ == Cone::Glaze) {
        throw KilnError("Cannot open door on glaze cone");
    }

    door_ = Door::Open;
}

void Kiln::close() {
    if (door_ == Door::Closed) {
        throw KilnError("Door is already closed");
    }

    door_ = Door::Closed;
}

void Kiln::fire() {
    if (door_ != Door::Closed) {
        throw KilnError("Cannot fire with open door");
    }

    if (cone_ == Cone::Glaze) {
        throw KilnError("Maximum temperature reached");
    }

    for (const Ware& ware : stack_) {
        if (ware.thickness_mm > kCrackThicknessMm) {
            throw KilnError("Ware is too thick");
        }
    }

    if (cone_ == Cone::Bisque) {
        for (const Ware& ware : stack_) {
            if (ware.program == Program::Bisque) {
                throw KilnError("Bisque ware cannot reach glaze cone");
            }
        }

        for (std::size_t i = 0; i < stack_.size(); ++i) {
            if (stack_[i].program == Program::Glaze) {
                glazeFired_[i] = true;
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
        throw KilnError("Kiln is already cold");
    }

    if (cone_ == Cone::Bisque) {
        for (std::size_t i = 0; i < stack_.size(); ++i) {
            if (stack_[i].program == Program::Glaze &&
                !glazeFired_[i]) {
                throw KilnError("Glaze ware has not been fired");
            }
        }

        cone_ = Cone::Cold;
    } else {
        cone_ = Cone::Bisque;
    }

    ++stokes_;
}

void Kiln::load(const Ware& ware) {
    if (door_ != Door::Open) {
        throw KilnError("Door must be open");
    }

    if (cone_ == Cone::Glaze) {
        throw KilnError("Cannot load on glaze cone");
    }

    if (cone_ == Cone::Bisque &&
        ware.program != Program::Glaze) {
        throw KilnError("Only glaze ware can be loaded on bisque cone");
    }

    if (validateWare(ware).has_value()) {
        throw KilnError("Invalid ware");
    }

    for (const Ware& current : stack_) {
        if (current.mark == ware.mark) {
            throw KilnError("Duplicate mark");
        }
    }

    if (ware.kind == WareKind::Sagger && !stack_.empty()) {
        throw KilnError("Sagger must occupy the kiln alone");
    }

    for (const Ware& current : stack_) {
        if (current.kind == WareKind::Sagger) {
            throw KilnError("Cannot load beside a sagger");
        }
    }

    if (!fitsInKiln(stack_, ware)) {
        throw KilnError("Ware does not fit in kiln");
    }

    stack_.push_back(ware);
    glazeFired_.push_back(false);
}

void Kiln::unload(const std::string& mark) {
    if (door_ != Door::Open) {
        throw KilnError("Door must be open");
    }

    if (stack_.empty()) {
        throw KilnError("Kiln is empty");
    }

    const Ware& top = stack_.back();

    if (top.mark != mark) {
        throw KilnError("Ware is not on top");
    }

    if (top.program == Program::Glaze &&
        cone_ != Cone::Cold) {
        throw KilnError("Glaze ware can only be unloaded when cold");
    }

    stack_.pop_back();
    glazeFired_.pop_back();
}

}  // namespace skopin