#include "skopin/kiln.hpp"

#include <algorithm>
#include <string>

namespace skopin {

namespace {

bool hasThickWare(const std::vector<Ware>& stack) {
    return std::any_of(stack.begin(), stack.end(), [](const Ware& ware) {
        return ware.thickness_mm > kCrackThicknessMm;
    });
}

bool hasBisque(const std::vector<Ware>& stack) {
    return std::any_of(stack.begin(), stack.end(), [](const Ware& ware) {
        return ware.program == Program::Bisque;
    });
}

bool hasSagger(const std::vector<Ware>& stack) {
    return std::any_of(stack.begin(), stack.end(), [](const Ware& ware) {
        return ware.kind == WareKind::Sagger;
    });
}

bool containsMark(const std::vector<Ware>& stack, const std::string& mark) {
    return std::any_of(stack.begin(), stack.end(), [&mark](const Ware& ware) {
        return ware.mark == mark;
    });
}

bool hasUnseenGlaze(const std::vector<Ware>& stack, const std::vector<bool>& glazed) {
    for (std::size_t index = 0; index < stack.size(); ++index) {
        if (stack[index].program == Program::Glaze && !glazed[index]) {
            return true;
        }
    }
    return false;
}

}  // namespace

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
        throw KilnError("дверь уже открыта");
    }
    if (cone_ == Cone::Glaze) {
        throw KilnError("слишком горячо, нельзя открыть дверь");
    }
    door_ = Door::Open;
}

void Kiln::close() {
    if (door_ == Door::Closed) {
        throw KilnError("дверь уже закрыта");
    }
    door_ = Door::Closed;
}

void Kiln::fire() {
    if (door_ != Door::Closed) {
        throw KilnError("необходимо закрыть дверь");
    }
    if (cone_ == Cone::Glaze) {
        throw KilnError("kiln уже в glaze cone");
    }
    if (hasThickWare(stack_)) {
        throw KilnError("ware слишком толстый");
    }
    if (cone_ == Cone::Bisque && hasBisque(stack_)) {
        throw KilnError("bisque нельзя готовить в glaze cone");
    }

    if (cone_ == Cone::Cold) {
        cone_ = Cone::Bisque;
    } else {
        cone_ = Cone::Glaze;
        for (std::size_t index = 0; index < glazed_.size(); index++) {
            glazed_[index] = true;
        }
    }
    stokes_++;
}

void Kiln::cool() {
    if (door_ != Door::Closed) {
        throw KilnError("необходимо закрыть дверь");
    }
    if (cone_ == Cone::Cold) {
        throw KilnError("kiln остыл");
    }
    if (cone_ == Cone::Bisque && hasUnseenGlaze(stack_, glazed_)) {
        throw KilnError("glaze должна побывать в glaze cone перед остыванием");
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
        throw KilnError("открой дверь перед load");
    }
    if (cone_ == Cone::Glaze) {
        throw KilnError("нияего нельзя загружать в glaze cone");
    }
    if (cone_ == Cone::Bisque && ware.program != Program::Glaze) {
        throw KilnError("только glaze может быть загружен в bisque cone");
    }

    const std::optional<std::string> error = validateWare(ware);
    if (error.has_value()) {
        throw KilnError(*error);
    }
    if (containsMark(stack_, ware.mark)) {
        throw KilnError("ware c mark уже внутри");
    }
    if (ware.kind == WareKind::Sagger && !stack_.empty()) {
        throw KilnError("sagger должен быть один в kiln");
    }
    if (hasSagger(stack_)) {
        throw KilnError("ничего не может быть загруженно рядом с sagger");
    }
    if (!fitsInKiln(stack_, ware)) {
        throw KilnError("ware не в kiln");
    }

    stack_.push_back(ware);
    glazed_.push_back(false);
}

void Kiln::unload(const std::string& mark) {
    if (door_ != Door::Open) {
        throw KilnError("открой дверь перед unload");
    }
    if (stack_.empty()) {
        throw KilnError("kiln пуст");
    }
    if (stack_.back().mark != mark) {
        throw KilnError("только верхний ware может быть unloaded");
    }
    if (stack_.back().program == Program::Glaze && cone_ != Cone::Cold) {
        throw KilnError("glaze может быть unload только из cold kiln");
    }

    stack_.pop_back();
    glazed_.pop_back();
}

}  // namespace skopin
