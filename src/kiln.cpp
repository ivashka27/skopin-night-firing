#include "skopin/kiln.hpp"

#include <stdexcept>

namespace skopin {

Kiln::Kiln() {}

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
        throw KilnError("too hot to open the door");
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
        throw KilnError("door must be closed");
    }
    if (cone_ == Cone::Glaze) {
        throw KilnError("already at the highest cone");
    }
    for (const Ware& w : stack_) {
        if (w.thickness_mm > kCrackThicknessMm) {
            throw KilnError("ware is too thick");
        }
    }
    if (cone_ == Cone::Bisque) {
        for (const Ware& w : stack_) {
            if (w.program == Program::Bisque) {
                throw KilnError("bisque ware would crack");
            }
        }
    }

    if (cone_ == Cone::Cold) {
        cone_ = Cone::Bisque;
    } else {
        cone_ = Cone::Glaze;
        for (std::size_t i = 0; i < seenGlaze_.size(); i++) {
            seenGlaze_[i] = true;
        }
    }
    stokes_++;
}

void Kiln::cool() {
    if (door_ == Door::Open) {
        throw KilnError("door must be closed");
    }
    if (cone_ == Cone::Cold) {
        throw KilnError("already cold");
    }
    if (cone_ == Cone::Bisque) {
        for (std::size_t i = 0; i < stack_.size(); i++) {
            if (stack_[i].program == Program::Glaze && !seenGlaze_[i]) {
                throw KilnError("glaze has not reached the glaze cone yet");
            }
        }
    }

    if (cone_ == Cone::Glaze) {
        cone_ = Cone::Bisque;
    } else {
        cone_ = Cone::Cold;
    }
    stokes_++;
}

void Kiln::load(const Ware& ware) {
    if (door_ != Door::Open) {
        throw KilnError("door must be open");
    }
    bool coneOk = (cone_ == Cone::Cold) ||
                  (cone_ == Cone::Bisque && ware.program == Program::Glaze);
    if (!coneOk) {
        throw KilnError("cone does not allow loading this ware");
    }
    if (validateWare(ware).has_value()) {
        throw KilnError("invalid ware");
    }
    for (const Ware& w : stack_) {
        if (w.mark == ware.mark) {
            throw KilnError("duplicate mark");
        }
    }
    if (ware.kind == WareKind::Sagger && !stack_.empty()) {
        throw KilnError("sagger needs an empty kiln");
    }
    if (!stack_.empty() && stack_[0].kind == WareKind::Sagger) {
        throw KilnError("nothing can be loaded with a sagger");
    }
    if (!fitsInKiln(stack_, ware)) {
        throw KilnError("does not fit by height");
    }

    stack_.push_back(ware);
    seenGlaze_.push_back(false);
}

void Kiln::unload(const std::string& mark) {
    if (door_ != Door::Open) {
        throw KilnError("door must be open");
    }
    if (stack_.empty()) {
        throw KilnError("kiln is empty");
    }
    if (stack_.back().mark != mark) {
        throw KilnError("not the top ware");
    }
    if (stack_.back().program == Program::Glaze && cone_ != Cone::Cold) {
        throw KilnError("glaze can only be unloaded cold");
    }

    stack_.pop_back();
    seenGlaze_.pop_back();
}

}  // namespace skopin
