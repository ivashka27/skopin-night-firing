#include "skopin/kiln.hpp"

namespace skopin {

Kiln::Kiln() {
    cone_ = Cone::Cold;
    door_ = Door::Closed;
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
    if (door_ == Door::Open) {
        throw KilnError("already open");
    }
    if (cone_ == Cone::Glaze) {
        throw KilnError("glaze cone");
    }
    door_ = Door::Open;
}

void Kiln::close() {
    if (door_ == Door::Closed) {
        throw KilnError("already closed");
    }
    door_ = Door::Closed;
}

void Kiln::fire() {
    // Отказ не тратит такт и не поднимает конус наполовину.
    if (door_ != Door::Closed) {
        throw KilnError("door open");
    }
    if (cone_ == Cone::Glaze) {
        throw KilnError("max cone");
    }
    for (std::size_t i = 0; i < stack_.size(); i++) {
        if (stack_[i].thickness_mm > kCrackThicknessMm) {
            throw KilnError("too thick");
        }
    }
    if (cone_ == Cone::Bisque) {
        for (std::size_t i = 0; i < stack_.size(); i++) {
            if (stack_[i].program == Program::Bisque) {
                throw KilnError("bisque inside");
            }
        }
    }

    if (cone_ == Cone::Cold) {
        cone_ = Cone::Bisque;
    } else if (cone_ == Cone::Bisque) {
        cone_ = Cone::Glaze;
        for (std::size_t i = 0; i < fired_glaze_.size(); i++) {
            fired_glaze_[i] = 1;
        }
    }
    stokes_++;
}

void Kiln::cool() {
    // Ровно один конус вниз. Полива, не видевшая поливного огня, остыть не даёт.
    if (door_ != Door::Closed) {
        throw KilnError("door open");
    }
    if (cone_ == Cone::Cold) {
        throw KilnError("min cone");
    }
    if (cone_ == Cone::Bisque) {
        for (std::size_t i = 0; i < stack_.size(); i++) {
            if (stack_[i].program == Program::Glaze && fired_glaze_[i] == 0) {
                throw KilnError("unfired glaze");
            }
        }
    }

    if (cone_ == Cone::Glaze) {
        cone_ = Cone::Bisque;
    } else if (cone_ == Cone::Bisque) {
        cone_ = Cone::Cold;
    }
    stokes_++;
}

void Kiln::load(const Ware& ware) {
    // Внутри должна оказаться копия. Дверцу load сам не открывает.
    if (door_ != Door::Open) {
        throw KilnError("door closed");
    }
    if (cone_ == Cone::Glaze) {
        throw KilnError("glaze cone");
    }
    if (cone_ == Cone::Bisque && ware.program != Program::Glaze) {
        throw KilnError("hot bisque");
    }
    if (validateWare(ware).has_value()) {
        throw KilnError("invalid ware");
    }
    for (std::size_t i = 0; i < stack_.size(); i++) {
        if (stack_[i].mark == ware.mark) {
            throw KilnError("duplicate mark");
        }
    }
    if (ware.kind == WareKind::Sagger && !stack_.empty()) {
        throw KilnError("not empty");
    }
    if (!stack_.empty() && stack_[0].kind == WareKind::Sagger) {
        throw KilnError("sagger inside");
    }
    if (!fitsInKiln(stack_, ware)) {
        throw KilnError("too tall");
    }

    stack_.push_back(ware);
    fired_glaze_.push_back(0);
}

void Kiln::unload(const std::string& mark) {
    // Снимается только верх стопки. Поливу с утильного конуса не снимают.
    if (door_ != Door::Open) {
        throw KilnError("door closed");
    }
    if (stack_.empty()) {
        throw KilnError("empty kiln");
    }
    if (stack_.back().mark != mark) {
        throw KilnError("wrong mark");
    }
    if (stack_.back().program == Program::Glaze && cone_ != Cone::Cold) {
        throw KilnError("still hot");
    }

    stack_.pop_back();
    fired_glaze_.pop_back();
}

}  // namespace skopin