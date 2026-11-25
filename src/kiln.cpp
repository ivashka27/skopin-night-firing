#include "skopin/kiln.hpp"

#include <stdexcept>

namespace skopin {

Kiln::Kiln() {
    cur_cone = Cone::Cold;
    cur_door = Door::Closed;
    cur_stokes = 0;
}

Cone Kiln::cone() const {
    return cur_cone;
}

Door Kiln::door() const {
    return cur_door;
}

int Kiln::stokes() const {
    return cur_stokes;
}

std::vector<Ware> Kiln::stack() const {
    return wares;
}

void Kiln::open() {
    if (cur_door == Door::Open) {
        throw KilnError("Door is already open");
    }
    if (cur_cone == Cone::Glaze) {
        throw KilnError("Cannot open door on glaze cone");
    }
    cur_door = Door::Open;
}

void Kiln::close() {
    if (cur_door == Door::Closed) {
        throw KilnError("Door is already closed");
    }
    cur_door = Door::Closed;
}

void Kiln::fire() {
    if (cur_door == Door::Open){
        throw KilnError("Door is Open");
    }
    if (cur_cone == Cone::Glaze){
        throw KilnError("Already on glaze cone");
    }

    for (const Ware& ware : wares) {
        if (ware.thickness_mm > kCrackThicknessMm) {
            throw KilnError ("Ware is too thick");
        }
    }

    if (cur_cone == Cone::Bisque) { 
        for (const Ware& ware : wares) {
            if (ware.program == Program::Bisque) {
                throw KilnError ("Bisque ware cannot reach glaze cone");
            }
        }
    }

    if (cur_cone == Cone::Cold){
        cur_cone = Cone::Bisque;
    }
    else if (cur_cone == Cone::Bisque){
        cur_cone = Cone::Glaze;
    
        for (const Ware& ware : wares) {
            if (ware.program == Program::Glaze) {
                glaze_reached_marks.push_back(ware.mark);
            }
        }
    }

    cur_stokes++;
}

void Kiln::cool() {
    if (cur_door == Door::Open) {
        throw KilnError("Door is open");
    }

    if (cur_cone == Cone::Bisque){
        for (const Ware& ware : wares){
            if (ware.program == Program::Glaze) {
                bool reached = false;

                for (const std::string& mark : glaze_reached_marks) {
                    if (mark == ware.mark) {
                        reached = true;
                    }
                }
                if (!reached) {
                    throw KilnError("Glaze ware must reach glaze cone");
                }
            }
        }
    }

    if (cur_cone == Cone::Cold){
            throw KilnError ("Alreadi is cold");
    }

    if (cur_cone == Cone::Glaze) {
        cur_cone = Cone::Bisque;
    }
    else if (cur_cone == Cone::Bisque) {
        cur_cone = Cone::Cold;
    }

    cur_stokes++;
}

void Kiln::load(const Ware& ware) {
    if (cur_door == Door::Closed) {
        throw KilnError("Door is closed");
    }
    std::optional<std::string> error = validateWare(ware);

    if (error.has_value()) {
        throw KilnError(*error);
    }
    for (const Ware& existing : wares) {
        if (existing.mark == ware.mark) {
            throw KilnError("Duplicate mark");
        }
    }

    if (cur_cone == Cone::Glaze) {
        throw KilnError("Cannot load on glaze cone");
    }
    if (cur_cone == Cone::Bisque && ware.program == Program::Bisque){
        throw KilnError ("Cannot load bisque ware on bisque cone");
    }

    if (ware.kind == WareKind::Sagger && !wares.empty()) {
        throw KilnError ("Sagger must be alone");
    }
    for (const Ware& existing : wares) {
        if (existing.kind == WareKind::Sagger) {
            throw KilnError("Cannot load ware with sagger");
        }
    }

    if (!fitsInKiln(wares, ware)){
        throw KilnError("Klin is full");
    }

     wares.push_back(ware);
}

void Kiln::unload(const std::string& mark) {
    if (cur_door == Door::Closed) {
        throw KilnError("Door id closed");
    }
    
    if (wares.empty()) {
        throw KilnError("Klin is empty");
    }

    if (wares.back().mark != mark){
        throw KilnError ("Only top ware can unloaded");
    }

    if (wares.back().program == Program::Glaze && cur_cone != Cone::Cold){
        throw KilnError("Glaze ware must cool to cold");
    }

    for (std::size_t i = 0; i < glaze_reached_marks.size(); i++) {
        if (glaze_reached_marks[i] == mark) {
            glaze_reached_marks.erase(glaze_reached_marks.begin() + i);
            break;
        }
    }

    wares.pop_back();
}

}  // namespace skopin
