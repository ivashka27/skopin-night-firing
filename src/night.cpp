#include "skopin/night.hpp"

#include "skopin/kiln.hpp"

#include <stdexcept>

namespace skopin {

NightReport runNightFire(const std::vector<Ware>& queue) {
    using Batch = std::vector<Ware>;

    NightReport report;
    std::vector<Ware> waiting;
    std::vector<std::string> seen;

    for (const Ware& ware : queue) {
        bool duplicate = false;
        for (const std::string& mark : seen) {
            if (mark == ware.mark) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) {
            report.refused.push_back({ware.mark, "duplicate"});
            continue;
        }
        seen.push_back(ware.mark);

        if (validateWare(ware).has_value()) {
            report.refused.push_back({ware.mark, "invalid"});
        } else if (ware.thickness_mm > kCrackThicknessMm) {
            report.refused.push_back({ware.mark, "thick"});
        } else {
            waiting.push_back(ware);
        }
    }

    std::vector<Batch> batches;
    while (!waiting.empty()) {
        const Ware base = waiting.front();
        Batch batch{base};
        std::vector<Ware> left;

        if (base.kind == WareKind::Sagger) {
            left.assign(waiting.begin() + 1, waiting.end());
        } else {
            bool skipped = false;
            std::size_t i = 1;
            for (; i < waiting.size(); ++i) {
                const Ware& ware = waiting[i];
                if (ware.kind == WareKind::Sagger || ware.program != base.program) {
                    break;
                }
                const bool mayJoin = !skipped || ware.kind == WareKind::Rush;
                if (mayJoin && fitsInKiln(batch, ware)) {
                    batch.push_back(ware);
                } else {
                    skipped = true;
                    left.push_back(ware);
                }
            }
            for (; i < waiting.size(); ++i) {
                left.push_back(waiting[i]);
            }
        }

        batches.push_back(batch);
        waiting = left;
    }

    Kiln kiln;

    auto loadAll = [&kiln](const Batch& batch) {
        for (auto it = batch.rbegin(); it != batch.rend(); ++it) {
            kiln.load(*it);
        }
    };
    auto unloadAll = [&kiln, &report](const Batch& batch) {
        for (const Ware& ware : batch) {
            kiln.unload(ware.mark);
            report.passed.push_back(ware.mark);
        }
    };

    for (std::size_t i = 0; i < batches.size(); ++i) {
        const Batch& batch = batches[i];
        const bool hasNext = i + 1 < batches.size();
        const bool nextIsGlaze = hasNext && batches[i + 1].front().program == Program::Glaze;

        if (batch.front().program == Program::Bisque) {
            if (kiln.door() == Door::Open) {
                kiln.close();
            }
            if (kiln.cone() == Cone::Bisque) {
                kiln.cool();
            }
            kiln.open();
            loadAll(batch);
            kiln.close();
            kiln.fire();
            kiln.open();
            unloadAll(batch);
            if (!nextIsGlaze) {
                kiln.close();
                kiln.cool();
            }
        } else {
            if (kiln.door() == Door::Closed) {
                kiln.open();
            }
            loadAll(batch);
            kiln.close();
            if (kiln.cone() == Cone::Cold) {
                kiln.fire();
            }
            kiln.fire();
            kiln.cool();
            kiln.cool();
            kiln.open();
            unloadAll(batch);
            kiln.close();
        }
    }

    report.stokes = kiln.stokes();
    return report;
}

}  // namespace skopin