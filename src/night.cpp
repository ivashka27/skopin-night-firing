#include "skopin/night.hpp"
#include "skopin/kiln.hpp"

#include <unordered_set>

namespace skopin {
NightReport runNightFire(const std::vector<Ware>& queue) {
    NightReport report;
    std::unordered_set<std::string> seen;
    std::vector<Ware> waiting;
    for (const Ware& ware : queue) {
        std::string reason;
        if (!seen.insert(ware.mark).second) reason = "duplicate";
        else if (validateWare(ware)) reason = "invalid";
        else if (ware.thickness_mm > kCrackThicknessMm) reason = "thick";
        if (reason.empty()) waiting.push_back(ware);
        else report.refused.push_back({ware.mark, reason});
    }

    Kiln kiln;
    while (!waiting.empty()) {
        std::vector<Ware> batch{waiting.front()};
        std::vector<Ware> remaining;
        const Program program = batch.front().program;
        bool skipped = false;
        bool wall = batch.front().kind == WareKind::Sagger;
        for (std::size_t i = 1; i < waiting.size(); ++i) {
            const Ware& ware = waiting[i];
            if (ware.program != program || ware.kind == WareKind::Sagger) wall = true;
            if (!wall && (!skipped || ware.kind == WareKind::Rush) &&
                fitsInKiln(batch, ware)) {
                batch.push_back(ware);
            } else {
                skipped = true;
                remaining.push_back(ware);
            }
        }
        waiting = std::move(remaining);

        if (program == Program::Bisque) {
            if (kiln.door() == Door::Open) kiln.close();
            if (kiln.cone() == Cone::Bisque) kiln.cool();
        }
        if (kiln.door() == Door::Closed) kiln.open();
        // Reverse loading makes the first chosen ware the top of the stack.
        for (auto it = batch.rbegin(); it != batch.rend(); ++it) kiln.load(*it);
        kiln.close();
        kiln.fire();
        if (program == Program::Glaze) {
            if (kiln.cone() != Cone::Glaze) kiln.fire();
            kiln.cool();
            kiln.cool();
        }
        kiln.open();
        for (const Ware& ware : batch) {
            kiln.unload(ware.mark);
            report.passed.push_back(ware.mark);
        }
        const bool hotNext = program == Program::Bisque && !waiting.empty() &&
                             waiting.front().program == Program::Glaze;
        if (!hotNext) {
            kiln.close();
            if (kiln.cone() == Cone::Bisque) kiln.cool();
        }
    }
    report.stokes = kiln.stokes();
    return report;
}
}  // namespace skopin
