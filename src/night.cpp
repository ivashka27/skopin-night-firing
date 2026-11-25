
#include "skopin/night.hpp"

#include "skopin/kiln.hpp"

namespace skopin {

NightReport runNightFire(const std::vector<Ware>& queue) {
    NightReport report;

    std::vector<std::string> marks;
    std::vector<Ware> waiting;

    // Проверяем изделия.
    for (const Ware& ware : queue) {
        bool duplicate = false;

        for (const std::string& mark : marks) {
            if (mark == ware.mark) {
                duplicate = true;
            }
        }

        if (duplicate) {
            report.refused.push_back({ware.mark, "duplicate"});
            continue;
        }

        marks.push_back(ware.mark);

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

    // Список изделий, которые ещё ждут обжига.
    std::vector<Ware> lst = waiting;

    // Здесь храним все садки.
    std::vector<std::vector<Ware>> batches;

    // Собираем садки.
    while (!lst.empty()) {
        std::vector<Ware> batch;

        Ware first = lst[0];
        batch.push_back(first);
        lst.erase(lst.begin());

        // Капсель всегда один в садке.
        if (first.kind != WareKind::Sagger) {
            bool skipped = false;
            int i = 0;

            while (i < lst.size()) {
                if (lst[i].program != first.program ||
                    lst[i].kind == WareKind::Sagger) {
                    break;
                }

                if (fitsInKiln(batch, lst[i]) &&
                    (!skipped || lst[i].kind == WareKind::Rush)) {
                    batch.push_back(lst[i]);
                    lst.erase(lst.begin() + i);
                } else {
                    skipped = true;
                    i++;
                }
            }
        }

        batches.push_back(batch);
    }

    // Проводим садки через горн.
    Kiln kiln;

    for (int i = 0; i < batches.size(); i++) {
        std::vector<Ware>& batch = batches[i];

        if (batch[0].program == Program::Bisque) {
            if (kiln.door() == Door::Open) {
                kiln.close();
            }

            if (kiln.cone() == Cone::Bisque) {
                kiln.cool();
            }

            kiln.open();

            // Загружаем с конца к началу.
            for (int j = batch.size() - 1; j >= 0; j--) {
                kiln.load(batch[j]);
            }

            kiln.close();
            kiln.fire();
            kiln.open();

            // Снимаем в исходном порядке.
            for (const Ware& ware : batch) {
                kiln.unload(ware.mark);
                report.passed.push_back(ware.mark);
            }

            // Если следующая садка поливная,
            // оставляем горн горячим.
            if (i + 1 < batches.size() &&
                batches[i + 1][0].program == Program::Glaze) {
                continue;
            }

            kiln.close();
            kiln.cool();
        } else {
            if (kiln.door() == Door::Closed) {
                kiln.open();
            }

            for (int j = batch.size() - 1; j >= 0; j--) {
                kiln.load(batch[j]);
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
