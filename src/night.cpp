#include "skopin/night.hpp"

#include "skopin/kiln.hpp"

namespace skopin {

NightReport runNightFire(const std::vector<Ware>& queue) {
    // 1. Слева направо вычеркнуть duplicate, затем invalid, затем thick.
    //    Вычеркнутые не стоят в очереди и не рвут садку.
    NightReport res;
    std::vector<std::string> seen;
    std::vector<Ware> q;

    for (std::size_t i = 0; i < queue.size(); i++) {
        bool dup = false;
        for (std::size_t j = 0; j < seen.size(); j++) {
            if (seen[j] == queue[i].mark) {
                dup = true;
                break;
            }
        }
        if (dup) {
            res.refused.push_back({queue[i].mark, "duplicate"});
            continue;
        }
        seen.push_back(queue[i].mark);

        if (validateWare(queue[i]).has_value()) {
            res.refused.push_back({queue[i].mark, "invalid"});
            continue;
        }
        if (queue[i].thickness_mm > kCrackThicknessMm) {
            res.refused.push_back({queue[i].mark, "thick"});
            continue;
        }

        q.push_back(queue[i]);
    }

    // 2. Пока кто-то ждёт, собрать одну садку по правилам README.
    //    На стопку сажать с конца к началу: первое взятое снимается первым.
    std::vector<std::vector<Ware>> parts;
    std::vector<int> used(q.size(), 0);

    while (true) {
        std::size_t first = q.size();
        for (std::size_t i = 0; i < q.size(); i++) {
            if (used[i] == 0) {
                first = i;
                break;
            }
        }
        if (first == q.size()) {
            break;
        }

        std::vector<Ware> cur;
        cur.push_back(q[first]);
        used[first] = 1;
        Program prog = q[first].program;

        if (q[first].kind != WareKind::Sagger) {
            bool has_skip = false;
            for (std::size_t j = first + 1; j < q.size(); j++) {
                if (used[j] == 1) {
                    continue;
                }
                if (q[j].program != prog || q[j].kind == WareKind::Sagger) {
                    break;
                }

                bool ok = false;
                if (!has_skip) {
                    ok = true;
                } else if (q[j].kind == WareKind::Rush) {
                    ok = true;
                }

                if (ok && fitsInKiln(cur, q[j])) {
                    cur.push_back(q[j]);
                    used[j] = 1;
                } else {
                    has_skip = true;
                }
            }
        }

        parts.push_back(cur);
    }

    // 3. Провести её объектом Kiln. После утиля, за которым сразу полива,
    //    горн не остужать. stokes взять из kiln.stokes() в конце.
    Kiln kiln;

    for (std::size_t b = 0; b < parts.size(); b++) {
        const std::vector<Ware>& cur = parts[b];

        if (cur[0].program == Program::Bisque) {
            if (kiln.door() == Door::Open) {
                kiln.close();
            }
            if (kiln.cone() == Cone::Bisque) {
                kiln.cool();
            }

            kiln.open();
            for (std::size_t i = cur.size(); i > 0; i--) {
                kiln.load(cur[i - 1]);
            }
            kiln.close();

            kiln.fire();

            kiln.open();
            for (std::size_t i = 0; i < cur.size(); i++) {
                kiln.unload(cur[i].mark);
                res.passed.push_back(cur[i].mark);
            }

            bool next_glaze = false;
            if (b + 1 < parts.size() && parts[b + 1][0].program == Program::Glaze) {
                next_glaze = true;
            }
            if (!next_glaze) {
                kiln.close();
                kiln.cool();
            }
        } else {
            if (kiln.door() == Door::Closed) {
                kiln.open();
            }

            for (std::size_t i = cur.size(); i > 0; i--) {
                kiln.load(cur[i - 1]);
            }
            kiln.close();

            if (kiln.cone() == Cone::Cold) {
                kiln.fire();
            }
            kiln.fire();

            kiln.cool();
            kiln.cool();

            kiln.open();
            for (std::size_t i = 0; i < cur.size(); i++) {
                kiln.unload(cur[i].mark);
                res.passed.push_back(cur[i].mark);
            }
            kiln.close();
        }
    }

    res.stokes = kiln.stokes();
    return res;
}

}  // namespace skopin