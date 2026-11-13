#include "skopin/kiln.hpp"
#include "skopin/ware.hpp"
#include <stdexcept>

namespace skopin {

Kiln::Kiln() = default;

Cone Kiln::cone() const {
    return cone_;
}

Door Kiln::door() const {
    return door_;
}

int Kiln::stokes() const {
    return strokes_;
}

std::vector<Ware> Kiln::stack() const {
    return stack_;
}

void Kiln::open() {
    if (door_ == Door::Open) {
        throw KilnError("error: already opened");
    }
    if (cone_ == Cone::Glaze) {
        throw KilnError("error: can't open");
    }
    door_ = Door::Open;
}

void Kiln::close() {
    if (door_ == Door::Closed) {
        throw KilnError("error: already closed");
    }
    door_ = Door::Closed;
}

void Kiln::fire() {
    if (door_ == Door::Open) {
        throw KilnError("error:can't fire with open door");
    }
    if (cone_ == Cone::Glaze){
        throw KilnError("error: already at max");
    }
    for (int i = 0; i < stack_.size(); i++) {
        if (stack_[i].thickness_mm > 12) {
            throw KilnError("error: declined, too thick");
        }
        if (stack_[i].program == Program::Bisque && cone_ == Cone::Bisque) {
            throw KilnError("error: declined");
        }
    }
    if (cone_ == Cone::Cold) {
        cone_ = Cone::Bisque;
    }
    else if (cone_ == Cone::Bisque) {
        cone_ = Cone::Glaze;
        glazed_ = true;
    }
    strokes_++;
}


void Kiln::cool() {
    if (door_ == Door::Open) {
        throw KilnError("error: can't cold with open door");
    }
    if (cone_ == Cone::Cold) {
        throw KilnError("error: already at min");
    }
    for (int i = 0; i < stack_.size(); i++) {
        if (stack_[i].program == Program::Glaze && glazed_ == false) {
            throw KilnError("error: declined");
        }
    }
    if (cone_ == Cone::Glaze) {
        cone_ = Cone::Bisque;
    }
    else if (cone_ == Cone::Bisque) {
        cone_ = Cone::Cold;
        glazed_ = false;
    }
    strokes_++;
}

void Kiln::load(const Ware& ware) {
    if (door_ == Door::Closed) {
        throw KilnError("error: door is closed");
    }
    if (cone_ == Cone::Glaze) {
        throw KilnError("error: declined");
    }
    if (cone_ == Cone::Bisque && ware.program != Program::Glaze) {
        throw KilnError("error: wrong program");
    }
    if (validateWare(ware).has_value() == true) {
        throw KilnError("error: invalid value");
    }
    for (int i = 0; i < stack_.size(); i++) {
        if (stack_[i].mark == ware.mark) {
            throw KilnError("error: dublicate");
        }
    }
    if (stack_.empty() == false) {
        if (ware.kind == WareKind::Sagger) {
            throw KilnError("error: must be alone");
        }
        if (stack_.back().kind == WareKind::Sagger) {
            throw KilnError("error: must be alone");
        } 
    }
    if (fitsInKiln(stack_,ware) == false) {
        throw KilnError("error: declined");
    }
    stack_.push_back(ware);
}

void Kiln::unload(const std::string& mark) {
    if (door_ == Door::Closed) {
        throw KilnError("error: door is closed");
    }
    if (stack_.empty() == true) {
        throw KilnError("error: empty");
    }
    if (stack_.back().mark != mark) {
        throw KilnError("error: wrong mark");
    }
    if (stack_.back().program == Program::Glaze && cone_ != Cone::Cold) {
        throw KilnError("error: declined");
    }
    if (stack_.back().program == Program::Bisque && cone_ == Cone::Glaze) {
        throw KilnError("error:declined");
    }
    stack_.pop_back();
}

}  // namespace skopin