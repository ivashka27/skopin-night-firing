
#include "skopin/night.hpp"

#include "skopin/kiln.hpp"

#include <unordered_set>
#include <vector>

namespace skopin {

NightReport runNightFire(const std::vector<Ware>& queue) {
    NightReport report;
    std::unordered_set<std::string> seen;

    // Сначала вычёркиваем неподходящие изделия.
    std::vector<Ware> pending;

    for (const Ware& ware : queue) {
        if (!seen.insert(ware.mark).second) {
            report.refused.push_back({ware.mark, "duplicate"});
        } else if (validateWare(ware).has_value()) {
            report.refused.push_back({ware.mark, "invalid"});
        } else if (ware.thickness_mm > kCrackThicknessMm) {
            report.refused.push_back({ware.mark, "thick"});
        } else {
            pending.push_back(ware);
        }
    }

    Kiln kiln;

    while (!pending.empty()) {
        std::vector<Ware> batch;
        std::vector<bool> selected(pending.size(), false);

        const Program program = pending.front().program;

        batch.push_back(pending.front());
        selected[0] = true;

        // Капсель занимает весь горн.
        if (pending.front().kind != WareKind::Sagger) {
            bool skipped = false;

            for (std::size_t i = 1; i < pending.size(); ++i) {
                const Ware& ware = pending[i];

                // Чужая программа и капсель — стены.
                if (ware.program != program || ware.kind == WareKind::Sagger) {
                    break;
                }

                const bool fits = fitsInKiln(batch, ware);

                if (fits && (!skipped || ware.kind == WareKind::Rush)) {
                    batch.push_back(ware);
                    selected[i] = true;
                } else {
                    skipped = true;
                }
            }
        }

        // Убираем выбранные изделия, сохраняя порядок остальных.
        std::vector<Ware> remaining;
        for (std::size_t i = 0; i < pending.size(); ++i) {
            if (!selected[i]) {
                remaining.push_back(pending[i]);
            }
        }
        pending = remaining;

        const bool nextIsGlaze =
            !pending.empty() && pending.front().program == Program::Glaze;

        if (program == Program::Bisque) {
            // Утильную садку начинаем с холодного горна.
            if (kiln.door() == Door::Open) {
                kiln.close();
            }

            if (kiln.cone() == Cone::Bisque) {
                kiln.cool();
            }

            kiln.open();

            // Загружаем с конца, чтобы первое изделие оказалось сверху.
            for (auto it = batch.rbegin(); it != batch.rend(); ++it) {
                kiln.load(*it);
            }

            kiln.close();
            kiln.fire();
            kiln.open();

            // Снимаем в порядке исходной садки.
            for (const Ware& ware : batch) {
                kiln.unload(ware.mark);
                report.passed.push_back(ware.mark);
            }

            // Если дальше полива — оставляем горн горячим.
            if (!nextIsGlaze) {
                kiln.close();
                kiln.cool();
            }
        } else {
            // Поливу можно сажать и на холодный, и на утильный конус.
            if (kiln.door() == Door::Closed) {
                kiln.open();
            }

            for (auto it = batch.rbegin(); it != batch.rend(); ++it) {
                kiln.load(*it);
            }

            kiln.close();

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
