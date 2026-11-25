
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
        throw KilnError("Close the door first");
    }

    if (cone_ == Cone::Glaze) {
        throw KilnError("Maximum heat reached");
    }

    for (const Ware& ware : stack_) {
        if (ware.thickness_mm > kCrackThicknessMm) {
            throw KilnError("Ware is too thick");
        }

        if (cone_ == Cone::Bisque &&
            ware.program == Program::Bisque) {
            throw KilnError("Bisque cannot reach glaze heat");
        }
    }

    if (cone_ == Cone::Cold) {
        cone_ = Cone::Bisque;
    } else {
        cone_ = Cone::Glaze;

        for (std::size_t i = 0; i < stack_.size(); ++i) {
            if (stack_[i].program == Program::Glaze) {
                glazeSeen_[i] = true;
            }
        }
    }

    ++stokes_;
}

void Kiln::cool() {
    if (door_ != Door::Closed) {
        throw KilnError("Close the door first");
    }

    if (cone_ == Cone::Cold) {
        throw KilnError("Kiln is already cold");
    }

    if (cone_ == Cone::Bisque) {
        for (std::size_t i = 0; i < stack_.size(); ++i) {
            if (stack_[i].program == Program::Glaze &&
                !glazeSeen_[i]) {
                throw KilnError("Glaze has not been fired");
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
        (cone_ == Cone::Bisque &&
         ware.program != Program::Glaze)) {
        throw KilnError("Wrong loading temperature");
    }

    if (validateWare(ware).has_value()) {
        throw KilnError("Invalid ware");
    }

    for (const Ware& item : stack_) {
        if (item.mark == ware.mark) {
            throw KilnError("Duplicate mark");
        }

        if (item.kind == WareKind::Sagger) {
            throw KilnError("Sagger occupies the kiln");
        }
    }

    if (ware.kind == WareKind::Sagger &&
        !stack_.empty()) {
        throw KilnError("Sagger must be alone");
    }

    if (!fitsInKiln(stack_, ware)) {
        throw KilnError("Not enough space");
    }

    stack_.push_back(ware);
    glazeSeen_.push_back(false);
}

void Kiln::unload(const std::string& mark) {
    if (door_ != Door::Open) {
        throw KilnError("Door is closed");
    }

    if (stack_.empty()) {
        throw KilnError("Kiln is empty");
    }

    const Ware& top = stack_.back();

    if (top.mark != mark) {
        throw KilnError("Only the top ware can be removed");
    }

    if (top.program == Program::Glaze &&
        cone_ != Cone::Cold) {
        throw KilnError("Glaze must cool before unloading");
    }

    stack_.pop_back();
    glazeSeen_.pop_back();
}

}  // namespace skopin
