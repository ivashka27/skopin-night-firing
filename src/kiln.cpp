#include "skopin/kiln.hpp"

#include <stdexcept>

namespace skopin {

Kiln::Kiln() {
    c = Cone::Cold;
    d = Door::Closed;
    t = 0;
}

Cone Kiln::cone() const {
    return c;
}

Door Kiln::door() const {
    return d;
}

int Kiln::stokes() const {
    return t;
}

std::vector<Ware> Kiln::stack() const {
    return s;
}

void Kiln::open() {
    if (d == Door::Open) {
        throw KilnError("err");
    }
    if (c == Cone::Glaze) {
        throw KilnError("err");
    }
    d = Door::Open;
}

void Kiln::close() {
    if (d == Door::Closed) {
        throw KilnError("err");
    }
    d = Door::Closed;
}

void Kiln::fire() {
    if (d == Door::Open) {
        throw KilnError("err");
    }
    if (c == Cone::Glaze) {
        throw KilnError("err");
    }
    for (int i = 0; i < (int)s.size(); i++) {
        if (s[i].thickness_mm > 12) {
            throw KilnError("err");
        }
    }
    if (c == Cone::Bisque) {
        for (int i = 0; i < (int)s.size(); i++) {
            if (s[i].program == Program::Bisque) {
                throw KilnError("err");
            }
        }
    }

    if (c == Cone::Cold) {
        c = Cone::Bisque;
    } else if (c == Cone::Bisque) {
        c = Cone::Glaze;
        for (int i = 0; i < (int)b.size(); i++) {
            b[i] = 1;
        }
    }
    t = t + 1;
}

void Kiln::cool() {
    if (d == Door::Open) {
        throw KilnError("err");
    }
    if (c == Cone::Cold) {
        throw KilnError("err");
    }
    if (c == Cone::Bisque) {
        for (int i = 0; i < (int)s.size(); i++) {
            if (s[i].program == Program::Glaze && b[i] == 0) {
                throw KilnError("err");
            }
        }
    }

    if (c == Cone::Glaze) {
        c = Cone::Bisque;
    } else if (c == Cone::Bisque) {
        c = Cone::Cold;
    }
    t = t + 1;
}

void Kiln::load(const Ware& w) {
    if (d == Door::Closed) {
        throw KilnError("err");
    }
    if (c == Cone::Glaze) {
        throw KilnError("err");
    }
    if (c == Cone::Bisque && w.program != Program::Glaze) {
        throw KilnError("err");
    }
    if (validateWare(w).has_value() == true) {
        throw KilnError("err");
    }
    for (int i = 0; i < (int)s.size(); i++) {
        if (s[i].mark == w.mark) {
            throw KilnError("err");
        }
    }
    if (w.kind == WareKind::Sagger && (int)s.size() > 0) {
        throw KilnError("err");
    }
    if ((int)s.size() > 0 && s[0].kind == WareKind::Sagger) {
        throw KilnError("err");
    }
    if (fitsInKiln(s, w) == false) {
        throw KilnError("err");
    }

    s.push_back(w);
    b.push_back(0);
}

void Kiln::unload(const std::string& mark) {
    if (d == Door::Closed) {
        throw KilnError("err");
    }
    if ((int)s.size() == 0) {
        throw KilnError("err");
    }
    int last = (int)s.size() - 1;
    if (s[last].mark != mark) {
        throw KilnError("err");
    }
    if (s[last].program == Program::Glaze && c != Cone::Cold) {
        throw KilnError("err");
    }

    s.pop_back();
    b.pop_back();
}

}  // namespace skopin