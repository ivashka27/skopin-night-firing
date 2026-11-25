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
    if (door_ == Door::Open ){
        throw KilnError("Дверца уже открыта");
    }
    if (cone_ == Cone::Glaze){
        throw KilnError("Включен поливной конус");
    }
    door_ = Door::Open;
}

void Kiln::close() {
    if (door_ == Door::Closed){
        throw KilnError("Дверца уже закрыта");
    }
    door_ = Door::Closed;
}

void Kiln::fire() {
    if (door_ == Door::Open){
        throw KilnError("Дверца еще открыта");
    }
    if (cone_ == Cone::Glaze){
        throw KilnError("Уже максимальный конус");
    }
    for (const Ware&ware : stack_){
        if (ware.thickness_mm > kCrackThicknessMm){
            throw KilnError("Толщина одного из изделий не подходит");
        }
    }
    for (const Ware&ware : stack_){
        if (ware.program == Program::Bisque && cone_ == Cone::Bisque){
            throw KilnError("При повышении конуса одно из изделий испортиться");
        }
    }
    if (cone_ == Cone::Cold) {
        cone_ = Cone::Bisque;
    }
    else {
        cone_ = Cone::Glaze;
        for (std::size_t i = 0; i < stack_.size(); i++) {
            if (stack_[i].program == Program::Glaze) {
                reachedGlaze_[i] = true;
            }
        }
    }
    stokes_ += 1;
}

void Kiln::cool() {
    if (door_ == Door::Open){
        throw KilnError("Дверца еще открыта");
    }
    if (cone_ == Cone::Cold){
        throw KilnError("Уже минимальный конус");}
    if (cone_ == Cone::Bisque){
        for (std::size_t i = 0; i < stack_.size(); i++){
            if (stack_[i].program == Program::Glaze && reachedGlaze_[i] == false){
                throw KilnError("Одно из изделий не было в Glaze конусе");
            }
        }
    }
    if (cone_ == Cone::Glaze){
        cone_ = Cone::Bisque;
    }
    else{
        cone_ = Cone::Cold;
    }
    stokes_ +=1;
}

void Kiln::load(const Ware&ware) {
    if (door_ == Door::Closed){
        throw KilnError("Дверца еще закрыта");
    }
    if (ware.program != Program::Glaze && cone_ == Cone::Bisque){
        throw KilnError("На текущем этапе нельзя загрузить изделие этого типа");
    }
    if (cone_ == Cone::Glaze){
        throw KilnError("На текущем этапе нельзя загрузить изделие");
    }
    if (validateWare(ware).has_value()){
        throw KilnError("Изделие не прошло проверку");
    }
    for (const Ware& obj: stack_){
        if (obj.mark == ware.mark){
            throw KilnError("Изделие с таким именем уже загружено");
        }
    }
    if (ware.kind == WareKind::Sagger && stack_.size() > 0){
        throw KilnError("В горне уже есть изделия, поэтому нельзя загрузить Sagger");
    }
    for (const Ware& obj: stack_){
        if (obj.kind == WareKind::Sagger){
            throw KilnError("Горн занят Sagger-ом");
        }
    }
    if (fitsInKiln(stack_, ware) == false){
        throw KilnError("Издели не помещается в горн");
    }
    stack_.push_back(ware);
    reachedGlaze_.push_back(false);
}

void Kiln::unload(const std::string& mark) {
    if (door_ == Door::Closed){
        throw KilnError("Дверца закрыта");
    }
    if (stack_.size() ==0){
        throw KilnError("Горн пуст");
    }
    if (mark != stack_.back().mark){
        throw KilnError("Название изделия не совпадает с верхним в стопке");
    }
    if (stack_.back().program == Program::Glaze && cone_ != Cone::Cold){
        throw KilnError("Этот вид изделий нельзя снять с режима отличного от Cold");
    }
    stack_.pop_back();
    reachedGlaze_.pop_back();
}

}  // namespace skopin
