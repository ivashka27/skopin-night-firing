#include "skopin/kiln.hpp"

namespace skopin {

Kiln::Kiln() {
    cone_ = Cone::Cold;
    door_ = Door::Closed;
    stokes_ = 0;
}

Cone Kiln::cone() const { return cone_; }
Door Kiln::door() const { return door_; }
int Kiln::stokes() const { return stokes_; }
std::vector<Ware> Kiln::stack() const { return stack_; }

void Kiln::open() {
    if (door_ == Door::Open || cone_ == Cone::Glaze) {
        throw KilnError("Cannot open door");
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
        throw KilnError("Cannot fire");
    }

    int n = stack_.size();
    for (int i = 0; i < n; i++) {
        if (stack_[i].thickness_mm > kCrackThicknessMm) {
            throw KilnError("Ware too thick");
        }
        if (cone_ == Cone::Bisque && stack_[i].program == Program::Bisque) {
            throw KilnError("Bisque cannot reach glaze cone");
        }
    }
    
    if (cone_ == Cone::Cold) {
        cone_ = Cone::Bisque;
    } 
    else {
        cone_ = Cone::Glaze;
        int n = glazed_.size();
        for (int i = 0; i < n; i++) {
            glazed_[i] = 1;
        }
    }
    stokes_ = stokes_ + 1;
}

void Kiln::cool() {
    if (door_ != Door::Closed || cone_ == Cone::Cold) {
        throw KilnError("Cannot cool");
    }
    if (cone_ == Cone::Bisque) {
        int n = stack_.size();
        for (int i = 0; i < n; i++) {
            if (stack_[i].program == Program::Glaze && glazed_[i] == 0) {
                throw KilnError("Glaze not fired");
            }
        }
        cone_ = Cone::Cold;
    } else {
        cone_ = Cone::Bisque;
    }
    stokes_ = stokes_ + 1;
}

void Kiln::load(const Ware& ware) {
    if (door_ != Door::Open) {
        throw KilnError("Door closed");
    }
    if (cone_ == Cone::Glaze ||
        (cone_ == Cone::Bisque && ware.program != Program::Glaze)) {
        throw KilnError("Wrong cone for loading");
    }
    std::optional<std::string> error = validateWare(ware);
    if (error.has_value()) {
        throw KilnError(error.value());
    }
    int n = stack_.size();
    for (int i = 0; i < n; i++) {
        if (stack_[i].mark == ware.mark) {
            throw KilnError("Duplicate mark");
        }
        if (stack_[i].kind == WareKind::Sagger || ware.kind == WareKind::Sagger) {
            throw KilnError("Sagger must be alone");
        }
    }
    if (!fitsInKiln(stack_, ware)) {
        throw KilnError("Not enough height");
    }
    stack_.push_back(ware);
    glazed_.push_back(0);
}

void Kiln::unload(const std::string& mark) {
    if (door_ != Door::Open || stack_ == "") {
        throw KilnError("Cannot unload");
    }
    int last = stack_.size() - 1;
    if (stack_[last].mark != mark) {
        throw KilnError("Wrong top mark");
    }
    if (stack_[last].program == Program::Glaze && cone_ != Cone::Cold) {
        throw KilnError("Glaze must be cold");
    }
    stack_.pop_back();
    glazed_.pop_back();
}

}  // namespace skopin
