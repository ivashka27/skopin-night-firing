
#include "skopin/kiln.hpp"

namespace skopin {

Kiln::Kiln() = default;

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
    if (door_ == Door::Open || cone_ == Cone::Glaze) {
        throw KilnError("Cannot open door");
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
    if (door_ != Door::Closed || cone_ == Cone::Glaze) {
        throw KilnError("Cannot fire");
    }

    for (const Ware& ware : stack_) {
        if (ware.thickness_mm > kCrackThicknessMm) {
            throw KilnError("Ware is too thick");
        }

        if (cone_ == Cone::Bisque && ware.program == Program::Bisque) {
            throw KilnError("Bisque ware cannot reach glaze cone");
        }
    }

    if (cone_ == Cone::Cold) {
        cone_ = Cone::Bisque;
    } else {
        cone_ = Cone::Glaze;

        for (std::size_t i = 0; i < stack_.size(); ++i) {
            if (stack_[i].program == Program::Glaze) {
                glazeReached_[i] = true;
            }
        }
    }

    ++stokes_;
}

void Kiln::cool() {
    if (door_ != Door::Closed || cone_ == Cone::Cold) {
        throw KilnError("Cannot cool");
    }

    if (cone_ == Cone::Bisque) {
        for (std::size_t i = 0; i < stack_.size(); ++i) {
            if (stack_[i].program == Program::Glaze && !glazeReached_[i]) {
                throw KilnError("Glaze has not reached glaze cone");
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
        throw KilnError("Door is closed");
    }

    if (cone_ == Cone::Glaze ||
        (cone_ == Cone::Bisque && ware.program != Program::Glaze)) {
        throw KilnError("Wrong cone for loading");
    }

    if (validateWare(ware).has_value()) {
        throw KilnError("Invalid ware");
    }

    for (const Ware& existing : stack_) {
        if (existing.mark == ware.mark) {
            throw KilnError("Duplicate mark");
        }
    }

    if (ware.kind == WareKind::Sagger && !stack_.empty()) {
        throw KilnError("Sagger must be alone");
    }

    for (const Ware& existing : stack_) {
        if (existing.kind == WareKind::Sagger) {
            throw KilnError("Sagger must be alone");
        }
    }

    if (!fitsInKiln(stack_, ware)) {
        throw KilnError("Ware does not fit");
    }

    stack_.push_back(ware);
    glazeReached_.push_back(false);
}

void Kiln::unload(const std::string& mark) {
    if (door_ != Door::Open || stack_.empty()) {
        throw KilnError("Cannot unload");
    }

    const Ware& top = stack_.back();

    if (top.mark != mark) {
        throw KilnError("Wrong mark");
    }

    if (top.program == Program::Glaze && cone_ != Cone::Cold) {
        throw KilnError("Glaze must be unloaded when cold");
    }

    stack_.pop_back();
    glazeReached_.pop_back();
}

}  // namespace skopin

