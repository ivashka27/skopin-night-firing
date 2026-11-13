#include "skopin/night.hpp"

#include "skopin/kiln.hpp"

#include <stdexcept>

namespace skopin {

NightReport runNightFire(const std::vector<Ware>& queue) {
    // 1. Слева направо вычеркнуть duplicate, затем invalid, затем thick.
    //    Вычеркнутые не стоят в очереди и не рвут садку.
    // 2. Пока кто-то ждёт, собрать одну садку по правилам README.
    //    На стопку сажать с конца к началу: первое взятое снимается первым.
    // 3. Провести её объектом Kiln. После утиля, за которым сразу полива,
    //    горн не остужать. stokes взять из kiln.stokes() в конце.
    NightReport report;
    report.stokes = 0;

    std::vector<std::string> seen_marks;
    std::vector<Ware> valid;

    for (const auto& item : queue){
        bool is_duplicate = false;
        for (const auto& m : seen_marks){
            if (item.mark == m){
                is_duplicate = true;
                break;
            }
        }
        if (is_duplicate){
            report.refused.push_back({item.mark, "duplicate"});
            continue;
        }
        seen_marks.push_back(item.mark);

        if (validateWare(item).has_value()){
            report.refused.push_back({item.mark, "invalid"});
            continue;
        }

        if (item.thickness_mm > 12){
            report.refused.push_back({item.mark, "thick"});
            continue;
        }
        valid.push_back(item);
    }
    std::vector<std::vector<Ware>> batches;
    while(!valid.empty()) {
        Ware base = valid[0];
        valid.erase(valid.begin());
        Program bP = base.program;
            if (base.kind == WareKind::Sagger) {
                batches.push_back({base});
                continue;
            }

            std::vector<Ware> batch;
        batch.push_back(base);
        bool skipped = false;

        size_t i = 0;
        while (i < valid.size()) {
            const Ware& w = valid[i];

            if (w.program != bP || w.kind == WareKind::Sagger) {
                break;
            }

            if (!skipped || w.kind == WareKind::Rush) {
                if (fitsInKiln(batch, w)) {
                    batch.push_back(w);
                    valid.erase(valid.begin() + i);
                    continue;
                }
            }
            skipped = true;
            i++;
        }
        batches.push_back(batch);
    }
    Kiln kiln;

    for (size_t b = 0; b < batches.size(); b++) {
        const auto& batch = batches[b];
        Program bP = batch[0].program;
        bool next_is_glaze = (b + 1 < batches.size() &&
            batches[b + 1][0].program == Program::Glaze);

        if (bP == Program::Bisque) {
            if (kiln.door() == Door::Open) {
                kiln.close();
            }
            if (kiln.cone() == Cone::Bisque) {
                kiln.cool();
            }

            kiln.open();
            for (auto it = batch.rbegin(); it != batch.rend(); ++it) {
                kiln.load(*it);
            }
            kiln.close();

            kiln.fire();

            kiln.open();
            for (const auto& w : batch) {
                kiln.unload(w.mark);
                report.passed.push_back(w.mark);
            }

            if (next_is_glaze) {
                continue;
            }

            kiln.close();
            kiln.cool();

        } else {
            if (kiln.door() == Door::Closed) {
                kiln.open();
            }

            for (auto it = batch.rbegin(); it != batch.rend(); ++it) {
                kiln.load(*it);
            }
            kiln.close();

            if (kiln.cone() == Cone::Cold) {
                kiln.fire();
                kiln.fire();
            } else {
                kiln.fire();
            }

            kiln.cool();
            kiln.cool();

            kiln.open();
            for (const auto& w : batch) {
                kiln.unload(w.mark);
                report.passed.push_back(w.mark);
            }
            kiln.close();
        }
    }

    report.stokes = kiln.stokes();
    return report;
}

}  // namespace skopin
