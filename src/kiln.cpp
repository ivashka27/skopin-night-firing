
#include "skopin/kiln.hpp"

#include <cstddef>

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
    if (door_ == Door::Open) {
        throw KilnError("Door is already open");
    }

    if (cone_ == Cone::Glaze) {
        throw KilnError("Kiln is too hot");
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
        throw KilnError("Door must be closed");
    }

    if (cone_ == Cone::Glaze) {
        throw KilnError("Maximum temperature reached");
    }

    for (const Ware& ware : stack_) {
        if (ware.thickness_mm > kCrackThicknessMm) {
            throw KilnError("Ware is too thick");
        }

        if (cone_ == Cone::Bisque &&
            ware.program == Program::Bisque) {
            throw KilnError("Bisque ware cannot reach glaze");
        }
    }

    if (cone_ == Cone::Cold) {
        cone_ = Cone::Bisque;
    } else {
        cone_ = Cone::Glaze;

        for (std::size_t i = 0; i < stack_.size(); ++i) {
            if (stack_[i].program == Program::Glaze) {
                reachedGlaze_[i] = true;
            }
        }
    }

    ++stokes_;
}

void Kiln::cool() {
    if (door_ != Door::Closed) {
        throw KilnError("Door must be closed");
    }

    if (cone_ == Cone::Cold) {
        throw KilnError("Kiln is already cold");
    }

    if (cone_ == Cone::Bisque) {
        for (std::size_t i = 0; i < stack_.size(); ++i) {
            if (stack_[i].program == Program::Glaze &&
                !reachedGlaze_[i]) {
                throw KilnError("Glaze ware was not fired");
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
        throw KilnError("Cannot load at glaze temperature");
    }

    if (cone_ == Cone::Bisque &&
        ware.program != Program::Glaze) {
        throw KilnError("Only glaze ware can be loaded hot");
    }

    if (validateWare(ware).has_value()) {
        throw KilnError("Invalid ware");
    }

    for (const Ware& inside : stack_) {
        if (inside.mark == ware.mark) {
            throw KilnError("Duplicate mark");
        }
    }

    if (ware.kind == WareKind::Sagger && !stack_.empty()) {
        throw KilnError("Sagger must be alone");
    }

    for (const Ware& inside : stack_) {
        if (inside.kind == WareKind::Sagger) {
            throw KilnError("Cannot load next to sagger");
        }
    }

    if (!fitsInKiln(stack_, ware)) {
        throw KilnError("Not enough space");
    }

    stack_.push_back(ware);
    reachedGlaze_.push_back(false);
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
        throw KilnError("Glaze ware must cool completely");
    }

    stack_.pop_back();
    reachedGlaze_.pop_back();
}
}  // namespace skopin
