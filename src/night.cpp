
#include "skopin/night.hpp"
#include "skopin/kiln.hpp"

#include <set>
#include <vector>

namespace skopin {

NightReport runNightFire(const std::vector<Ware>& queue) {
    NightReport report;
    Kiln kiln;

    std::set<std::string> seen;
    std::vector<Ware> waiting;

    // 1. Проверяем исходную очередь.
    for (const Ware& ware : queue) {
        if (!seen.insert(ware.mark).second) {
            report.refused.push_back(
                {ware.mark, "duplicate"}
            );
        } else if (validateWare(ware).has_value()) {
            report.refused.push_back(
                {ware.mark, "invalid"}
            );
        } else if (ware.thickness_mm > kCrackThicknessMm) {
            report.refused.push_back(
                {ware.mark, "thick"}
            );
        } else {
            waiting.push_back(ware);
        }
    }

    // 2. Обрабатываем партии, пока очередь не пуста.
    while (!waiting.empty()) {
        std::vector<Ware> batch;
        std::vector<bool> selected(waiting.size(), false);

        batch.push_back(waiting[0]);
        selected[0] = true;

        Program program = waiting[0].program;

        // Капсель всегда обжигается один.
        if (waiting[0].kind != WareKind::Sagger) {
            bool skipped = false;

            for (std::size_t i = 1; i < waiting.size(); ++i) {
                const Ware& ware = waiting[i];

                // Другая программа или капсель - стена.
                if (ware.program != program ||
                    ware.kind == WareKind::Sagger) {
                    break;
                }

                bool canTake =
                    (!skipped || ware.kind == WareKind::Rush) &&
                    fitsInKiln(batch, ware);

                if (canTake) {
                    batch.push_back(ware);
                    selected[i] = true;
                } else {
                    skipped = true;
                }
            }
        }

        // 3. Убираем выбранные изделия из очереди.
        std::vector<Ware> remaining;

        for (std::size_t i = 0; i < waiting.size(); ++i) {
            if (!selected[i]) {
                remaining.push_back(waiting[i]);
            }
        }

        waiting = remaining;

        // Проверяем, будет ли следующая партия поливной.
        bool nextIsGlaze =
            !waiting.empty() &&
            waiting[0].program == Program::Glaze;

        // 4. Обжиг утильной партии.
        if (program == Program::Bisque) {
            if (kiln.door() == Door::Open) {
                kiln.close();
            }

            if (kiln.cone() == Cone::Bisque) {
                kiln.cool();
            }

            kiln.open();

            // Кладём в обратном порядке.
            for (auto it = batch.rbegin();
                 it != batch.rend(); ++it) {
                kiln.load(*it);
            }

            kiln.close();
            kiln.fire();

            kiln.open();

            for (const Ware& ware : batch) {
                kiln.unload(ware.mark);
                report.passed.push_back(ware.mark);
            }

            // Перед поливой сохраняем горячую печь.
            if (!nextIsGlaze) {
                kiln.close();
                kiln.cool();
            }
        }

        // 5. Обжиг поливной партии.
        else {
            if (kiln.door() == Door::Closed) {
                kiln.open();
            }

            for (auto it = batch.rbegin();
                 it != batch.rend(); ++it) {
                kiln.load(*it);
            }

            kiln.close();

            // С холодного конуса нужно два нагрева,
            // с утильного - один.
            if (kiln.cone() == Cone::Cold) {
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
