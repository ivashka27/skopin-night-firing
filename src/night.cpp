#include "skopin/night.hpp"
#include "skopin/kiln.hpp"

#include <unordered_set>
#include <utility>
#include <vector>

namespace skopin {

NightReport runNightFire(const std::vector<Ware>& queue) {
    NightReport report;
    Kiln kiln;

    std::unordered_set<std::string> seen;
    std::vector<Ware> waiting;

    for (const Ware& ware : queue) {
        if (!seen.insert(ware.mark).second) {
            report.refused.push_back({ware.mark, "duplicate"});
        } else if (validateWare(ware).has_value()) {
            report.refused.push_back({ware.mark, "invalid"});
        } else if (ware.thickness_mm > kCrackThicknessMm) {
            report.refused.push_back({ware.mark, "thick"});
        } else {
            waiting.push_back(ware);
        }
    }

    while (!waiting.empty()) {
        std::vector<Ware> batch{waiting.front()};
        std::vector<Ware> remaining;

        bool blocked = batch.front().kind == WareKind::Sagger;
        bool skipped = false;

        for (std::size_t i = 1; i < waiting.size(); ++i) {
            const Ware& ware = waiting[i];

            if (blocked ||
                ware.program != batch.front().program ||
                ware.kind == WareKind::Sagger) {
                blocked = true;
                remaining.push_back(ware);
                continue;
            }

            if ((!skipped || ware.kind == WareKind::Rush) &&
                fitsInKiln(batch, ware)) {
                batch.push_back(ware);
            } else {
                skipped = true;
                remaining.push_back(ware);
            }
        }

        waiting = std::move(remaining);

        if (batch.front().program == Program::Bisque) {
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

            if (waiting.empty() ||
                waiting.front().program != Program::Glaze) {
                kiln.close();
                kiln.cool();
            }
        } else {
            if (kiln.door() == Door::Closed) {
                kiln.open();
            }

            bool hot = kiln.cone() == Cone::Bisque;

            for (auto it = batch.rbegin(); it != batch.rend(); ++it) {
                kiln.load(*it);
            }

            kiln.close();

            if (!hot) {
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
