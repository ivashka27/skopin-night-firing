#include "skopin/kiln.hpp"

#include <stdexcept>

namespace skopin {

Kiln::Kiln() {
    cone_ = Cone::Cold;
    door_ = Door::Closed;
    stack_.clear();
    stokes_ = 0;
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
    // Повторное открытие и открытие на поливном конусе — KilnError.
    if (door_ == Door::Open){
        throw KilnError("door is already open");
    }
    if (cone_ == Cone::Glaze){
        throw KilnError("cone is too hot");
    }
    door_ = Door::Open;
}

void Kiln::close() {
    if (door_ == Door::Closed){
        throw KilnError("door is already closed");
    }
    door_ = Door::Closed;
}

void Kiln::fire() {
    // Отказ не тратит такт и не поднимает конус наполовину.
    if (door_ == Door::Open){
        throw KilnError("door must be closed");
    }
    for (const auto& w : stack_){
        if (w.thickness_mm > 12){
            throw KilnError("wall too thick");
        }
    }
    if (cone_ == Cone::Cold){
        cone_ = Cone::Bisque;
        stokes_ += 1;
    }
    else if (cone_ == Cone::Bisque){
        for (const auto& w : stack_){
            if (w.program == Program::Bisque){
                throw KilnError("bisque ware cannot reach glaze temperature");
            }
        }
        cone_ = Cone::Glaze;
        for (const auto& w : stack_){
            glaze_fired_.push_back(w.mark);
        }
        stokes_ += 1;
    }
    else {
        throw KilnError("cone already at max temperature");
    }
}

void Kiln::cool() {
    // Ровно один конус вниз. Полива, не видевшая поливного огня, остыть не даёт.
    if (door_ == Door::Open){
        throw KilnError("door must be closed");
    }
    if (cone_ == Cone::Glaze){
        cone_ = Cone::Bisque;
        stokes_ += 1;
    }
    else if (cone_ == Cone::Bisque){
        for (const auto& w: stack_){
            if (w.program == Program::Glaze){
                bool SeenGlaze = false;
                for (const auto& name : glaze_fired_){
                    if (name == w.mark){
                        SeenGlaze = true;
                        break;
                    }
                }
                if (!SeenGlaze){
                    throw KilnError("Glaze that not seen glase");
                }
            }
        }
        cone_ = Cone::Cold;
        stokes_ += 1;
    }
    else {
        throw KilnError("cone already at min temperature");
    }
}

void Kiln::load(const Ware& w) {
    // Внутри должна оказаться копия. Дверцу load сам не открывает.
    if (door_ != Door::Open) {
        throw KilnError("Error1");
    }
    if (cone_ == Cone::Glaze) {
        throw KilnError("Error2");
    }
    if (cone_ == Cone::Bisque && w.program == Program::Bisque) {
        throw KilnError("Error3");
    }
    if (validateWare(w).has_value()){
        throw KilnError("Error4");
    }
    for (const auto& item: stack_){
        if (item.mark == w.mark){
            throw KilnError("Error5");
        }
    }
    if (w.kind == WareKind::Sagger && !stack_.empty()){
        throw KilnError("Error6");
    }
    for (const auto& item : stack_){
        if (item.kind == WareKind::Sagger){
            throw KilnError("Error7");
        }
    }
    if (!(fitsInKiln(stack_ , w))){
        throw KilnError("Error8");
    }
    stack_.push_back(w);
}

void Kiln::unload(const std::string& m) {
    // Снимается только верх стопки. Поливу с утильного конуса не снимают.
    if (stack_.empty()){
        throw KilnError("Error9");
    }
    if (door_ == Door::Closed){
        throw KilnError("Error10");
    }
    if (m == stack_.back().mark){
        if (stack_.back().program == Program::Glaze && cone_ == Cone::Bisque){
            throw KilnError("Поливу c утильного конуса не снимают");
        }
        stack_.pop_back();
    } else{
        throw KilnError("Wrong mark");
    }
}

}  // namespace skopin
