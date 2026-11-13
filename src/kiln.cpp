#include "skopin/kiln.hpp"
#include "skopin/measures.hpp"
#include "skopin/ware.hpp"
#include <string>
#include <vector>

namespace skopin {

namespace {

bool hasThickWare(const std::vector<Ware>& stack) {
    for (const Ware& ware : stack) {
        if (ware.thickness_mm > kCrackThicknessMm) {
            return true;
        }
    }
    return false;
}

bool hasBisqueWare(const std::vector<Ware>& stack) {
    for (const Ware& ware : stack) {
        if (ware.program == Program::Bisque) {
            return true;
        }
    }
    return false;
}

bool hasUnseenGlazeWare(const std::vector<Ware>& stack,
                        const std::vector<bool>& seen) {
    const int n = static_cast<int>(stack.size());
    for (int i = 0; i < n; ++i) {
        if (stack[i].program == Program::Glaze && !seen[i]) {
            return true;
        }
    }
    return false;
}

bool hasMark(const std::vector<Ware>& stack, const std::string& mark) {
    for (const Ware& ware : stack) {
        if (ware.mark == mark) {
            return true;
        }
    }
    return false;
}

bool hasSagger(const std::vector<Ware>& stack) {
    for (const Ware& ware : stack) {
        if (ware.kind == WareKind::Sagger) {
            return true;
        }
    }
    return false;
}

}

Kiln::Kiln() = default;

Cone Kiln::cone() const { return cone_; }
Door Kiln::door() const { return door_; }
int Kiln::stokes() const { return stokes_; }
std::vector<Ware> Kiln::stack() const { return stack_; }

void Kiln::open() {
    if (door_ == Door::Open) {
        throw KilnError("open: door already open");
    }
    if (cone_ == Cone::Glaze) {
        throw KilnError("open: too hot to open");
    }
    door_ = Door::Open;
}

void Kiln::close() {
    if (door_ == Door::Closed) {
        throw KilnError("close: door already closed");
    }
    door_ = Door::Closed;
}

void Kiln::fire() {
    if (door_ == Door::Open) {
        throw KilnError("fire: door must be closed");
    }
    if (cone_ == Cone::Glaze) {
        throw KilnError("fire: already at glaze");
    }
    if (hasThickWare(stack_)) {
        throw KilnError("fire: thick ware inside");
    }
    if (cone_ == Cone::Bisque && hasBisqueWare(stack_)) {
        throw KilnError("fire: bisque ware would crack");
    }

    if (cone_ == Cone::Cold) {
        cone_ = Cone::Bisque;
    } else {
        cone_ = Cone::Glaze;
        for (int i = 0; i < static_cast<int>(seen_glaze_.size()); ++i) {
            seen_glaze_[i] = true;
        }
    }
    ++stokes_;
}

void Kiln::cool() {
    if (door_ == Door::Open) {
        throw KilnError("cool: door must be closed");
    }
    if (cone_ == Cone::Cold) {
        throw KilnError("cool: already cold");
    }
    if (cone_ == Cone::Bisque && hasUnseenGlazeWare(stack_, seen_glaze_)) {
        throw KilnError("cool: glaze has not seen the glaze cone");
    }

    if (cone_ == Cone::Glaze) {
        cone_ = Cone::Bisque;
    } else {
        cone_ = Cone::Cold;
    }
    ++stokes_;
}

void Kiln::load(const Ware& ware) {
    if (door_ == Door::Closed) {
        throw KilnError("load: door must be open");
    }
    if (cone_ == Cone::Glaze) {
        throw KilnError("load: too hot to load anything");
    }
    if (cone_ == Cone::Bisque && ware.program != Program::Glaze) {
        throw KilnError("load: only glaze can be loaded at the bisque cone");
    }
    if (validateWare(ware).has_value()) {
        throw KilnError("load: invalid ware");
    }
    if (hasMark(stack_, ware.mark)) {
        throw KilnError("load: duplicate mark");
    }
    if (ware.kind == WareKind::Sagger && !stack_.empty()) {
        throw KilnError("load: sagger needs an empty kiln");
    }
    if (hasSagger(stack_)) {
        throw KilnError("load: sagger occupies the kiln");
    }
    if (!fitsInKiln(stack_, ware)) {
        throw KilnError("load: does not fit");
    }

    stack_.push_back(ware);
    seen_glaze_.push_back(false);
}

void Kiln::unload(const std::string& mark) {
    if (door_ == Door::Closed) {
        throw KilnError("unload: door must be open");
    }
    if (stack_.empty()) {
        throw KilnError("unload: kiln is empty");
    }
    if (stack_.back().mark != mark) {
        throw KilnError("unload: not the top of the stack");
    }
    if (stack_.back().program == Program::Glaze && cone_ != Cone::Cold) {
        throw KilnError("unload: glaze can only be unloaded at the cold cone");
    }

    stack_.pop_back();
    seen_glaze_.pop_back();
    }
    }