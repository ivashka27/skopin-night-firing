#include "skopin/night.hpp"
#include "skopin/kiln.hpp"
#include "skopin/measures.hpp"
#include "skopin/ware.hpp"
#include <string>
#include <vector>

namespace skopin {

namespace {

std::vector<Ware> collectBatch(std::vector<Ware>& remaining) {
    std::vector<Ware> batch;
    if (remaining.empty()) {
        return batch;
    }

    Ware base = remaining.front();
    remaining.erase(remaining.begin());
    batch.push_back(base);

    if (base.kind == WareKind::Sagger) {
        return batch;
    }

    const Program program = base.program;
    bool skipped = false;

    int i = 0;
    while (i < static_cast<int>(remaining.size())) {
        const Ware& candidate = remaining[i];

        if (candidate.program != program || candidate.kind == WareKind::Sagger) {
            break;
        }

        if (fitsInKiln(batch, candidate)) {
            if (!skipped || candidate.kind == WareKind::Rush) {
                batch.push_back(candidate);
                remaining.erase(remaining.begin() + i);
                continue;
            }
        } else {
            skipped = true;
        }
        ++i;
    }

    return batch;
}

}

NightReport runNightFire(const std::vector<Ware>& queue) {
    NightReport report;

    // 1. Фильтрация очереди.
    std::vector<std::string> seen_marks;
    std::vector<Ware> remaining;

    for (const Ware& ware : queue) {
        bool duplicate = false;
        for (const std::string& mark : seen_marks) {
            if (mark == ware.mark) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) {
            report.refused.push_back({ware.mark, "duplicate"});
            continue;
        }
        seen_marks.push_back(ware.mark);

        if (validateWare(ware).has_value()) {
            report.refused.push_back({ware.mark, "invalid"});
            continue;
        }
        if (ware.thickness_mm > kCrackThicknessMm) {
            report.refused.push_back({ware.mark, "thick"});
            continue;
        }
        remaining.push_back(ware);
    }

    Kiln kiln;

    while (!remaining.empty()) {
        std::vector<Ware> batch = collectBatch(remaining);
        const Program program = batch.front().program;

        if (program == Program::Bisque) {
            // Подготовка: закрыть дверцу и остудить, если остались горячими.
            if (kiln.door() == Door::Open) {
                kiln.close();
            }
            if (kiln.cone() == Cone::Bisque) {
                kiln.cool();
            }

            kiln.open();
            for (int i = static_cast<int>(batch.size()) - 1; i >= 0; --i) {
                kiln.load(batch[i]);
            }
            kiln.close();
            kiln.fire();
            kiln.open();
            for (const Ware& ware : batch) {
                kiln.unload(ware.mark);
                report.passed.push_back(ware.mark);
            }

            bool next_is_glaze =
                !remaining.empty() && remaining.front().program == Program::Glaze;
            if (!next_is_glaze) {
                kiln.close();
                kiln.cool();
            }
        } else {
            if (kiln.door() == Door::Closed) {
                kiln.open();
            }
            for (int i = static_cast<int>(batch.size()) - 1; i >= 0; --i) {
                kiln.load(batch[i]);
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

    }