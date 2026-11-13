#include "skopin/kiln.hpp"
#include "skopin/measures.hpp"
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
    return stokes_;
}

std::vector<Ware> Kiln::stack() const {
    std::vector<Ware> result;
    result.reserve(slots_.size());
    for (const auto& slot : slots_) {
        result.push_back(slot.ware);
    }
    return result;
}

void Kiln::open() {
    if (door_ == Door::Open){
        throw KilnError("Door is already open");
    }
    if (cone_ == Cone::Glaze){
        throw KilnError("Cannot open door at Glaze cone");
    }
    door_ = Door::Open;
}

void Kiln::close() {
    if (door_ == Door::Closed) {
        throw KilnError("Door is already closed");
    }
    door_ = Door::Closed;
}

void Kiln::fire() {
    if (door_ != Door::Closed) {
        throw KilnError("Cannot fire with open door");
    }
    if (cone_ == Cone::Glaze) {
        throw KilnError("Already at maximum Glaze cone");
    }
    for (const auto& slot : slots_) {
        if (slot.ware.thickness_mm > kCrackThicknessMm) {
            throw KilnError("Ware is too thick to fire");
        }
        if (cone_ == Cone::Bisque && slot.ware.program == Program::Bisque) {
            throw KilnError("Bisque ware cannot be fired to Glaze cone");
        }
    }

    if (cone_ == Cone::Cold) {
        cone_ = Cone::Bisque;
    } else {
        cone_ = Cone::Glaze;
        for (auto& slot : slots_) {
            if (slot.ware.program == Program::Glaze) {
                slot.reached_glaze = true;
            }
        }
    }
    ++stokes_;
}

void Kiln::cool() {
    if (door_ != Door::Closed) {
        throw KilnError("Cannot cool with open door");
    }
    if (cone_ == Cone::Cold) {
        throw KilnError("Already at minimum Cold cone");
    }
    if (cone_ == Cone::Bisque) {
        for (const auto& slot : slots_) {
            if (slot.ware.program == Program::Glaze && !slot.reached_glaze) {
                throw KilnError("Unfired Glaze ware cannot be cooled to Cold");
            }
        }
        cone_ = Cone::Cold;
    } else {
        cone_ = Cone::Bisque;
    }
    ++stokes_;
}

void Kiln::load(const Ware& ware) {
    if (door_ != Door::Open) {
        throw KilnError("Cannot load with closed door");
    }
    if (cone_ == Cone::Glaze) {
        throw KilnError("Cannot load at Glaze cone");
    }
    if (cone_ == Cone::Bisque && ware.program != Program::Glaze) {
        throw KilnError("Only Glaze ware can be loaded at Bisque cone");
    }
    if (validateWare(ware).has_value()) {
        throw KilnError("Invalid ware");
    }
    for (const auto& slot : slots_) {
        if (slot.ware.mark == ware.mark) {
            throw KilnError("Duplicate mark in kiln");
        }
    }
    if (!slots_.empty()) {
        if (ware.kind == WareKind::Sagger || slots_.front().ware.kind == WareKind::Sagger) {
            throw KilnError("Sagger must occupy kiln alone");
        }
    }
    if (!fitsInKiln(stack(), ware)) {
        throw KilnError("Ware does not fit in kiln");
    }

    slots_.push_back(Slot{ware, false});
}

void Kiln::unload(const std::string& mark) {
    if (door_ != Door::Open) {
        throw KilnError("Cannot unload with closed door");
    }
    if (slots_.empty()) {
        throw KilnError("Kiln is empty");
    }
    const Ware& top = slots_.back().ware;
    if (top.mark != mark) {
        throw KilnError("Mark does not match top ware");
    }
    if (top.program == Program::Glaze && cone_ != Cone::Cold) {
        throw KilnError("Glaze ware can only be unloaded at Cold cone");
    }

    slots_.pop_back();
}

}  // namespace skopin
