#include "skopin/kiln.hpp"

#include <algorithm>

namespace skopin {

Kiln::Kiln()
    : cone_(Cone::Cold),
      door_(Door::Closed),
      stokes_(0),
      stack_(),
      glaze_fired_(false) {}

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
        throw KilnError("door already open");
    }

    if (cone_ == Cone::Glaze) {
        throw KilnError("cannot open at glaze cone");
    }

    door_ = Door::Open;
}

void Kiln::close() {
    if (door_ == Door::Closed) {
        throw KilnError("door already closed");
    }

    door_ = Door::Closed;
}

void Kiln::fire() {
    if (door_ != Door::Closed) {
        throw KilnError("door is open");
    }

    if (cone_ == Cone::Glaze) {
        throw KilnError("maximum cone reached");
    }

    for (const Ware& ware : stack_) {
        if (ware.thickness_mm > kCrackThicknessMm) {
            throw KilnError("ware too thick");
        }

        if (cone_ == Cone::Bisque &&
            ware.program == Program::Bisque) {
            throw KilnError("bisque ware cannot reach glaze cone");
        }
    }

    if (cone_ == Cone::Cold) {
        cone_ = Cone::Bisque;
    } else {
        cone_ = Cone::Glaze;
        glaze_fired_ = true;
    }

    ++stokes_;
}

void Kiln::cool() {
    if (door_ != Door::Closed) {
        throw KilnError("door is open");
    }

    if (cone_ == Cone::Cold) {
        throw KilnError("kiln is already cold");
    }

    if (cone_ == Cone::Bisque) {
        for (const Ware& ware : stack_) {
            if (ware.program == Program::Glaze && !glaze_fired_) {
                throw KilnError("glaze ware has not reached glaze cone");
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
        throw KilnError("door is closed");
    }

    if (cone_ == Cone::Glaze) {
        throw KilnError("cannot load at glaze cone");
    }

    if (cone_ == Cone::Bisque && ware.program != Program::Glaze) {
        throw KilnError("only glaze ware can be loaded on bisque cone");
    }

    if (validateWare(ware)) {
        throw KilnError("invalid ware");
    }

    for (const Ware& existing : stack_) {
        if (existing.mark == ware.mark) {
            throw KilnError("duplicate mark");
        }
    }

    if (ware.kind == WareKind::Sagger && !stack_.empty()) {
        throw KilnError("sagger occupies the kiln alone");
    }

    if (!stack_.empty() && stack_.back().kind == WareKind::Sagger) {
        throw KilnError("cannot load beside a sagger");
    }

    if (!fitsInKiln(stack_, ware)) {
        throw KilnError("kiln is full");
    }

    stack_.push_back(ware);

    if (ware.program == Program::Glaze) {
        glaze_fired_ = false;
    }
}

void Kiln::unload(const std::string& mark) {
    if (door_ != Door::Open) {
        throw KilnError("door is closed");
    }

    if (stack_.empty()) {
        throw KilnError("kiln is empty");
    }

    if (stack_.back().mark != mark) {
        throw KilnError("wrong ware");
    }

    if (stack_.back().program == Program::Glaze &&
        cone_ != Cone::Cold) {
        throw KilnError("glaze ware can only be unloaded when cold");
    }

    stack_.pop_back();
}

}  // namespace skopin