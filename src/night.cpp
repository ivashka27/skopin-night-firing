#include "skopin/night.hpp"

#include "skopin/kiln.hpp"

#include <stdexcept>

namespace skopin {

NightReport runNightFire(const std::vector<Ware>&queue) {
    // 1. Слева направо вычеркнуть duplicate, затем invalid, затем thick.
    //    Вычеркнутые не стоят в очереди и не рвут садку.
    // 2. Пока кто-то ждёт, собрать одну садку по правилам README.
    //    На стопку сажать с конца к началу: первое взятое снимается первым.
    // 3. Провести её объектом Kiln. После утиля, за которым сразу полива,
    //    горн не остужать. stokes взять из kiln.stokes() в конце.
    NightReport report;

    std::vector<Ware> waiting;
    std::vector<std::string> usedMarks;

    for (const Ware& ware : queue) {
        bool duplicate = false;

        for (const std::string& mark : usedMarks) {
            if (mark == ware.mark) {
                duplicate = true;
                break;
            }
        }

        if (duplicate) {
            report.refused.push_back({ware.mark, "duplicate"});
            continue;
        }

        usedMarks.push_back(ware.mark);

        if (validateWare(ware).has_value()) {
            report.refused.push_back({ware.mark, "invalid"});
            continue;
        }

        if (ware.thickness_mm > kCrackThicknessMm) {
            report.refused.push_back({ware.mark, "thick"});
            continue;
        }

        waiting.push_back(ware);
    }

    Kiln kiln;

    while (!waiting.empty()) {
        std::vector<Ware> batch;
        std::vector<bool> selected(waiting.size(), false);

        batch.push_back(waiting[0]);
        selected[0] = true;

        Program program = waiting[0].program;
        bool skipped = false;

        if (waiting[0].kind != WareKind::Sagger) {
            for (std::size_t i = 1; i < waiting.size(); ++i) {
                if (waiting[i].program != program ||
                    waiting[i].kind == WareKind::Sagger) {
                    break;
                }

                bool canTake = !skipped ||
                              waiting[i].kind == WareKind::Rush;

                if (canTake && fitsInKiln(batch, waiting[i])) {
                    batch.push_back(waiting[i]);
                    selected[i] = true;
                } else {
                    skipped = true;
                }
            }
        }

        std::vector<Ware> remaining;

        for (std::size_t i = 0; i < waiting.size(); ++i) {
            if (!selected[i]) {
                remaining.push_back(waiting[i]);
            }
        }

        waiting = remaining;

        if (program == Program::Bisque) {
            if (kiln.door() == Door::Open) {
                kiln.close();
            }

            if (kiln.cone() == Cone::Bisque) {
                kiln.cool();
            }

            kiln.open();

            for (std::size_t i = batch.size(); i > 0; --i) {
                kiln.load(batch[i - 1]);
            }

            kiln.close();
            kiln.fire();
            kiln.open();

            for (const Ware& ware : batch) {
                kiln.unload(ware.mark);
                report.passed.push_back(ware.mark);
            }

            if (waiting.empty() ||
                waiting[0].program != Program::Glaze) {
                kiln.close();
                kiln.cool();
            }
        }
        else {
            if (kiln.door() == Door::Closed) {
                kiln.open();
            }

            bool cold = kiln.cone() == Cone::Cold;

            for (std::size_t i = batch.size(); i > 0; --i) {
                kiln.load(batch[i - 1]);
            }

            kiln.close();

            if (cold) {
                kiln.fire();
            }

            kiln.fire();

            kiln.cool();
            kiln.cool();

            kiln.open();

            for (const Ware& ware : batch) {
                kiln.unload(ware.mark);
                report.passed.push_back(ware.mark);
            }

            kiln.close();
        }
    }

    report.stokes = kiln.stokes();

    return report;

}

}  // namespace skopin
