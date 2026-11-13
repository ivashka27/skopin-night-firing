#include "skopin/night.hpp"

#include "skopin/kiln.hpp"

#include <string>
#include <vector>

namespace skopin {

namespace {

bool containsMark(const std::vector<std::string>& marks, const std::string& mark) {
    for (const std::string& known : marks) {
        if (known == mark) {
            return true;
        }
    }
    return false;
}

std::vector<Ware> removeBadWares(const std::vector<Ware>& queue, std::vector<Refusal>& refused) {
    std::vector<Ware> waiting;
    std::vector<std::string> seen_marks;

    for (const Ware& ware : queue) {
        if (containsMark(seen_marks, ware.mark)) {
            refused.push_back({ware.mark, "duplicate"});
            continue;
        }
        seen_marks.push_back(ware.mark);

        if (validateWare(ware).has_value()) {
            refused.push_back({ware.mark, "invalid"});
            continue;
        }
        if (ware.thickness_mm > kCrackThicknessMm) {
            refused.push_back({ware.mark, "thick"});
            continue;
        }

        waiting.push_back(ware);
    }

    return waiting;
}

std::vector<std::vector<Ware>> makeBatches(const std::vector<Ware>& waiting) {
    std::vector<std::vector<Ware>> batches;
    std::vector<Ware> remaining = waiting;

    while (!remaining.empty()) {
        std::vector<Ware> batch;
        batch.push_back(remaining[0]);
        const Program program = remaining[0].program;

        if (remaining[0].kind == WareKind::Sagger) {
            batches.push_back(batch);
            remaining.erase(remaining.begin());
            continue;
        }

        bool skipped_someone = false;
        std::vector<Ware> next_remaining;

        for (std::size_t index = 1; index < remaining.size(); index++) {
            const Ware& candidate = remaining[index];

            if (candidate.program != program || candidate.kind == WareKind::Sagger) {
                for (std::size_t rest = index; rest < remaining.size(); rest++) {
                    next_remaining.push_back(remaining[rest]);
                }
                break;
            }

            const bool fits = fitsInKiln(batch, candidate);
            const bool can_take = fits && (!skipped_someone || candidate.kind == WareKind::Rush);

            if (can_take) {
                batch.push_back(candidate);
            } else {
                skipped_someone = true;
                next_remaining.push_back(candidate);
            }
        }

        batches.push_back(batch);
        remaining = next_remaining;
    }

    return batches;
}

void fireBatch(Kiln& kiln, const std::vector<Ware>& batch, bool next_is_glaze, NightReport& report) {
    if (batch[0].program == Program::Bisque) {
        if (kiln.door() == Door::Open) {
            kiln.close();
        }
        if (kiln.cone() == Cone::Bisque) {
            kiln.cool();
        }

        kiln.open();
        for (std::size_t index = batch.size(); index > 0; index--) {
            kiln.load(batch[index - 1]);
        }
        kiln.close();

        kiln.fire();

        kiln.open();
        for (const Ware& ware : batch) {
            kiln.unload(ware.mark);
            report.passed.push_back(ware.mark);
        }

        if (!next_is_glaze) {
            kiln.close();
            kiln.cool();
        }
    } else {
        if (kiln.door() == Door::Closed) {
            kiln.open();
        }

        for (std::size_t index = batch.size(); index > 0; index--) {
            kiln.load(batch[index - 1]);
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

}  // namespace

NightReport runNightFire(const std::vector<Ware>& queue) {
    NightReport report;

    const std::vector<Ware> waiting = removeBadWares(queue, report.refused);
    const std::vector<std::vector<Ware>> batches = makeBatches(waiting);

    Kiln kiln;
    for (std::size_t index = 0; index < batches.size(); index++) {
        const bool next_is_glaze =
            index + 1 < batches.size() && batches[index + 1][0].program == Program::Glaze;
        fireBatch(kiln, batches[index], next_is_glaze, report);
    }

    report.stokes = kiln.stokes();
    return report;
}

}  // namespace skopin
