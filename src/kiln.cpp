#include "skopin/kiln.hpp"

namespace skopin {
Kiln::Kiln() = default;
Cone Kiln::cone() const { return cone_; }
Door Kiln::door() const { return door_; }
int Kiln::stokes() const { return stokes_; }
std::vector<Ware> Kiln::stack() const { return stack_; }

void Kiln::open() {
    if (door_ == Door::Open || cone_ == Cone::Glaze)
        throw KilnError("Cannot open door");
    door_ = Door::Open;
}
void Kiln::close() {
    if (door_ == Door::Closed) throw KilnError("Door already closed");
    door_ = Door::Closed;
}
void Kiln::fire() {
    if (door_ != Door::Closed || cone_ == Cone::Glaze)
        throw KilnError("Cannot raise heat");
    for (const Ware& ware : stack_) {
        if (ware.thickness_mm > kCrackThicknessMm)
            throw KilnError("Ware too thick");
        if (cone_ == Cone::Bisque && ware.program == Program::Bisque)
            throw KilnError("Bisque cannot reach glaze heat");
    }
    cone_ = cone_ == Cone::Cold ? Cone::Bisque : Cone::Glaze;
    if (cone_ == Cone::Glaze) {
        for (std::size_t i = 0; i < glazed_.size(); ++i) glazed_[i] = true;
    }
    ++stokes_;
}
void Kiln::cool() {
    if (door_ != Door::Closed || cone_ == Cone::Cold)
        throw KilnError("Cannot lower heat");
    if (cone_ == Cone::Bisque) {
        for (std::size_t i = 0; i < stack_.size(); ++i) {
            if (stack_[i].program == Program::Glaze && !glazed_[i])
                throw KilnError("Glaze has not been fired yet");
        }
    }
    cone_ = cone_ == Cone::Glaze ? Cone::Bisque : Cone::Cold;
    ++stokes_;
}
void Kiln::load(const Ware& ware) {
    if (door_ != Door::Open || cone_ == Cone::Glaze ||
        (cone_ == Cone::Bisque && ware.program != Program::Glaze))
        throw KilnError("Cannot load at this heat or door state");
    if (validateWare(ware)) throw KilnError("Invalid ware");
    for (const Ware& inside : stack_) {
        if (inside.mark == ware.mark) throw KilnError("Duplicate mark");
        if (inside.kind == WareKind::Sagger || ware.kind == WareKind::Sagger)
            throw KilnError("Sagger must stand alone");
    }
    if (!fitsInKiln(stack_, ware)) throw KilnError("Not enough height");
    glazed_.push_back(false);
    try {
        stack_.push_back(ware);
    } catch (...) {
        glazed_.pop_back();
        throw;
    }
}
void Kiln::unload(const std::string& mark) {
    if (door_ != Door::Open || stack_.empty() || stack_.back().mark != mark)
        throw KilnError("Cannot unload this mark");
    if (stack_.back().program == Program::Glaze && cone_ != Cone::Cold)
        throw KilnError("Glaze must be cold");
    stack_.pop_back();
    glazed_.pop_back();
}
}  // namespace skopin
