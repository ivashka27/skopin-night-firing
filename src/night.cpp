#include "skopin/night.hpp"

#include "skopin/kiln.hpp"

#include <stdexcept>

namespace skopin {

NightReport runNightFire(const std::vector<Ware>& q) {
    NightReport res;
    res.stokes = 0;

    std::vector<std::string> m;
    std::vector<Ware> a;

    for (int i = 0; i < (int)q.size(); i++) {
        int dup = 0;
        for (int j = 0; j < (int)m.size(); j++) {
            if (m[j] == q[i].mark) {
                dup = 1;
            }
        }

        if (dup == 1) {
            Refusal r;
            r.mark = q[i].mark;
            r.reason = "duplicate";
            res.refused.push_back(r);
        } else {
            m.push_back(q[i].mark);

            if (validateWare(q[i]).has_value() == true) {
                Refusal r;
                r.mark = q[i].mark;
                r.reason = "invalid";
                res.refused.push_back(r);
            } else if (q[i].thickness_mm > 12) {
                Refusal r;
                r.mark = q[i].mark;
                r.reason = "thick";
                res.refused.push_back(r);
            } else {
                a.push_back(q[i]);
            }
        }
    }

    Kiln k;

    while ((int)a.size() > 0) {
        std::vector<Ware> cur;
        std::vector<Ware> ost;

        cur.push_back(a[0]);

        int f = 0;
        int stena = 0;
        if (a[0].kind == WareKind::Sagger) {
            stena = 1;
        }

        for (int i = 1; i < (int)a.size(); i++) {
            if (a[i].program != a[0].program || a[i].kind == WareKind::Sagger) {
                stena = 1;
            }

            if (stena == 1) {
                ost.push_back(a[i]);
            } else {
                int ok = 0;
                if (f == 0) {
                    ok = 1;
                }
                if (a[i].kind == WareKind::Rush) {
                    ok = 1;
                }

                if (ok == 1 && fitsInKiln(cur, a[i]) == true) {
                    cur.push_back(a[i]);
                } else {
                    f = 1;
                    ost.push_back(a[i]);
                }
            }
        }

        a = ost;

        if (cur[0].program == Program::Bisque) {
            if (k.door() == Door::Open) {
                k.close();
            }
            if (k.cone() == Cone::Bisque) {
                k.cool();
            }

            k.open();
            for (int i = (int)cur.size() - 1; i >= 0; i--) {
                k.load(cur[i]);
            }
            k.close();

            k.fire();

            k.open();
            for (int i = 0; i < (int)cur.size(); i++) {
                k.unload(cur[i].mark);
                res.passed.push_back(cur[i].mark);
            }

            if ((int)a.size() > 0 && a[0].program == Program::Glaze) {
                // оставляем горячим
            } else {
                k.close();
                k.cool();
            }
        } else {
            if (k.door() == Door::Closed) {
                k.open();
            }

            for (int i = (int)cur.size() - 1; i >= 0; i--) {
                k.load(cur[i]);
            }
            k.close();

            if (k.cone() == Cone::Cold) {
                k.fire();
                k.fire();
            } else {
                k.fire();
            }

            k.cool();
            k.cool();

            k.open();
            for (int i = 0; i < (int)cur.size(); i++) {
                k.unload(cur[i].mark);
                res.passed.push_back(cur[i].mark);
            }
            k.close();
        }
    }

    res.stokes = k.stokes();
    return res;
}

}  // namespace skopin