
#include "skopin/night.hpp"

#include "skopin/kiln.hpp"

#include <unordered_set>
#include <vector>

namespace skopin {

NightReport runNightFire(const std::vector<Ware>& queue) {
    NightReport report;

    std::unordered_set<std::string> seen;

    std::vector<Ware> accepted;

    for (const Ware& ware : queue) {

        if (!seen.insert(ware.mark).second) {
            report.refused.push_back({ware.mark, "duplicate"});
            continue;
        }

        if (validateWare(ware).has_value()) {
            report.refused.push_back({ware.mark, "invalid"});
            continue;
        }

        if (ware.thickness_mm > kCrackThicknessMm) {
            report.refused.push_back({ware.mark, "thick"});
            continue;
        }

        accepted.push_back(ware);
    }

    std::vector<std::vector<Ware>> batches;

    std::vector<bool> used(accepted.size(), false);

    while (true) {

        std::size_t first = 0;

        while (first < accepted.size() && used[first]) {
            ++first;
        }

        if (first == accepted.size()) {
            break;
        }

        std::vector<Ware> batch;

        batch.push_back(accepted[first]);
        used[first] = true;

        if (accepted[first].kind == WareKind::Sagger) {
            batches.push_back(batch);
            continue;
        }

        bool skipped = false;

        for (std::size_t i = first + 1; i < accepted.size(); ++i) {

            if (used[i]) {
                continue;
            }

            const Ware& candidate = accepted[i];

            if (candidate.program != accepted[first].program ||
                candidate.kind == WareKind::Sagger) {
                break;
            }

            const bool fits = fitsInKiln(batch, candidate);

            if (fits && (!skipped || candidate.kind == WareKind::Rush)) {
                batch.push_back(candidate);
                used[i] = true;
            } else {
                skipped = true;
            }
        }

        batches.push_back(batch);
    }

    Kiln kiln;

    for (std::size_t batchIndex = 0;
         batchIndex < batches.size();
         ++batchIndex) {

        const std::vector<Ware>& batch = batches[batchIndex];

        const Program program = batch.front().program;

        if (program == Program::Bisque) {

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

            for (const Ware& ware : batch) {
                kiln.unload(ware.mark);
                report.passed.push_back(ware.mark);
            }

            const bool nextIsGlaze =
                batchIndex + 1 < batches.size() &&
                batches[batchIndex + 1].front().program == Program::Glaze;

            if (!nextIsGlaze) {
                kiln.close();
                kiln.cool();
            }

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