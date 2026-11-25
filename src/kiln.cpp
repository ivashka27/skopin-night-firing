#include "skopin/kiln.hpp"

#include <stdexcept>

namespace skopin {

Kiln::Kiln() {
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
        throw KilnError("door is already open");
    }

    if (cone_ == Cone::Glaze) {
        throw KilnError("cannot open on glaze cone");
    }

    door_ = Door::Open;
}

void Kiln::close() {
    if (door_ == Door::Closed) {
        throw KilnError("door is already closed");
    }

    door_ = Door::Closed;
}

void Kiln::fire() {
    if (door_ == Door::Open) {
        throw KilnError("door is open");
    }

    if (cone_ == Cone::Glaze) {
        throw KilnError("already at glaze");
    }

    for (const Ware& ware : stack_) {
        if (ware.thickness_mm > kCrackThicknessMm) {
            throw KilnError("ware is too thick");
        }
    }

    if (cone_ == Cone::Bisque) {
        for (const Ware& ware : stack_) {
            if (ware.program == Program::Bisque) {
                throw KilnError("bisque ware on glaze firing");
            }
        }

        cone_ = Cone::Glaze;

        for (int i = 0; i < stack_.size(); i++) {
            if (stack_[i].program == Program::Glaze) {
                fired_[i] = true;
            }
        }
    } else {
        cone_ = Cone::Bisque;
    }

    stokes_++;
}

void Kiln::cool() {
    if (door_ == Door::Open) {
        throw KilnError("door is open");
    }

    if (cone_ == Cone::Cold) {
        throw KilnError("already cold");
    }

    if (cone_ == Cone::Bisque) {
        for (int i = 0; i < stack_.size(); i++) {
            if (stack_[i].program == Program::Glaze && !fired_[i]) {
                throw KilnError("glaze was not fired");
            }
        }

        cone_ = Cone::Cold;
    } else {
        cone_ = Cone::Bisque;
    }

    stokes_++;
}

void Kiln::load(const Ware& ware) {
    if (door_ == Door::Closed) {
        throw KilnError("door is closed");
    }

    if (cone_ == Cone::Glaze) {
        throw KilnError("cannot load on glaze cone");
    }

    if (cone_ == Cone::Bisque && ware.program == Program::Bisque) {
        throw KilnError("wrong program");
    }

    if (validateWare(ware).has_value()) {
        throw KilnError("invalid ware");
    }

    if (!fitsInKiln(stack_, ware)) {
        throw KilnError("does not fit");
    }

    if (ware.kind == WareKind::Sagger && !stack_.empty()) {
        throw KilnError("sagger must be alone");
    }

    if (!stack_.empty() && stack_[0].kind == WareKind::Sagger) {
        throw KilnError("sagger must be alone");
    }

    for (const Ware& item : stack_) {
        if (item.mark == ware.mark) {
            throw KilnError("duplicate mark");
        }
    }

    stack_.push_back(ware);
    fired_.push_back(false);
}

void Kiln::unload(const std::string& mark) {
    if (door_ == Door::Closed) {
        throw KilnError("door is closed");
    }

    if (stack_.empty()) {
        throw KilnError("kiln is empty");
    }

    if (stack_.back().mark != mark) {
        throw KilnError("wrong mark");
    }

    if (stack_.back().program == Program::Glaze && cone_ != Cone::Cold) {
        throw KilnError("glaze is still hot");
    }

    stack_.pop_back();
    fired_.pop_back();
}

}  // namespace skopin
