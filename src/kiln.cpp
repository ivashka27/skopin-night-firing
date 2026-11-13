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
        throw KilnError("Дверца уже открыта");
    }
    if (cone_ == Cone::Glaze) {
        throw KilnError("На поливном конусе дверцу открыть нельзя");
    }
    door_ = Door::Open;
}

void Kiln::close() {
    if (door_ == Door::Closed) {
        throw KilnError("Дверца уже закрыта");
    }
    door_ = Door::Closed;
}

void Kiln::fire() {
    if (door_ == Door::Open) {
        throw KilnError("Для нагрева закройте дверцу");
    }
    if (cone_ == Cone::Glaze) {
        throw KilnError("Горн уже на поливном конусе");
    }
    for (const Ware& ware : stack_) {
        if (ware.thickness_mm > kCrackThicknessMm) {
            throw KilnError("Слишком толстая стенка изделия");
        }
        if (cone_ == Cone::Bisque && ware.program == Program::Bisque) {
            throw KilnError("Утиль нельзя нагревать до поливного конуса");
        }
    }

    if (cone_ == Cone::Cold) {
        cone_ = Cone::Bisque;
    } else {
        cone_ = Cone::Glaze;
        pendingGlazes_ = 0;
    }
    ++stokes_;
}

void Kiln::cool() {
    if (door_ == Door::Open) {
        throw KilnError("Для охлаждения закройте дверцу");
    }
    if (cone_ == Cone::Cold) {
        throw KilnError("Горн уже холодный");
    }
    if (cone_ == Cone::Bisque && pendingGlazes_ != 0) {
        throw KilnError("Сначала поднимите поливу до поливного конуса");
    }

    cone_ = cone_ == Cone::Glaze ? Cone::Bisque : Cone::Cold;
    ++stokes_;
}

void Kiln::load(const Ware& ware) {
    if (door_ == Door::Closed) {
        throw KilnError("Для загрузки откройте дверцу");
    }
    if (cone_ == Cone::Bisque && ware.program != Program::Glaze) {
        throw KilnError("В горячий горн можно загрузить только поливу");
    }
    if (const auto error = validateWare(ware)) {
        throw KilnError(*error);
    }
    for (const Ware& loaded : stack_) {
        if (loaded.mark == ware.mark) {
            throw KilnError("Изделие с таким клеймом уже в горне");
        }
    }
    if (!stack_.empty() &&
        (ware.kind == WareKind::Sagger || stack_.front().kind == WareKind::Sagger)) {
        throw KilnError("Капсель занимает горн целиком");
    }
    if (!fitsInKiln(stack_, ware)) {
        throw KilnError("Изделие не помещается по высоте");
    }

    stack_.push_back(ware);
    if (ware.program == Program::Glaze) {
        ++pendingGlazes_;
    }
}

void Kiln::unload(const std::string& mark) {
    if (door_ == Door::Closed) {
        throw KilnError("Для снятия откройте дверцу");
    }
    if (stack_.empty() || stack_.back().mark != mark) {
        throw KilnError("Снять можно только верхнее изделие");
    }

    if (stack_.back().program == Program::Glaze) {
        if (cone_ != Cone::Cold) {
            throw KilnError("Поливу можно снять только из холодного горна");
        }
        if (pendingGlazes_ != 0) {
            --pendingGlazes_;
        }
    }
    stack_.pop_back();
}

}  // namespace skopin
