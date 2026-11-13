#include "skopin/night.hpp"

#include "skopin/kiln.hpp"

#include <list>
#include <unordered_set>

namespace skopin {
namespace {

std::vector<Ware> takeBatch(std::list<Ware>& waiting) {
    std::vector<Ware> batch{waiting.front()};
    waiting.pop_front();
    if (batch.front().kind == WareKind::Sagger) {
        return batch;
    }

    const Program program = batch.front().program;
    bool skipped = false;
    for (auto it = waiting.begin(); it != waiting.end();) {
        const Ware& ware = *it;
        if (ware.program != program || ware.kind == WareKind::Sagger) {
            break;
        }
        if ((!skipped || ware.kind == WareKind::Rush) && fitsInKiln(batch, ware)) {
            batch.push_back(ware);
            it = waiting.erase(it);
        } else {
            skipped = true;
            ++it;
        }
    }
    return batch;
}

}  // namespace

NightReport runNightFire(const std::vector<Ware>& queue) {
    NightReport report;
    std::unordered_set<std::string> seenMarks;
    std::list<Ware> waiting;
    for (const Ware& ware : queue) {
        if (!seenMarks.insert(ware.mark).second) {
            report.refused.push_back({ware.mark, "duplicate"});
        } else if (validateWare(ware)) {
            report.refused.push_back({ware.mark, "invalid"});
        } else if (ware.thickness_mm > kCrackThicknessMm) {
            report.refused.push_back({ware.mark, "thick"});
        } else {
            waiting.push_back(ware);
        }
    }

    Kiln kiln;
    while (!waiting.empty()) {
        const auto batch = takeBatch(waiting);
        const bool isGlaze = batch.front().program == Program::Glaze;

        if (kiln.door() == Door::Closed) {
            kiln.open();
        }
        // Первое изделие партии окажется сверху и будет снято первым.
        for (auto it = batch.rbegin(); it != batch.rend(); ++it) {
            kiln.load(*it);
        }
        kiln.close();

        if (kiln.cone() == Cone::Cold) {
            kiln.fire();
        }
        if (isGlaze) {
            kiln.fire();
            kiln.cool();
            kiln.cool();
        }

        kiln.open();
        for (const Ware& ware : batch) {
            kiln.unload(ware.mark);
            report.passed.push_back(ware.mark);
        }

        if (!isGlaze && !waiting.empty() && waiting.front().program == Program::Glaze) {
            continue;
        }
        kiln.close();
        if (!isGlaze) {
            kiln.cool();
        }
    }

    report.stokes = kiln.stokes();
    return report;
}

}  // namespace skopin
