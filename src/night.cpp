#include "skopin/night.hpp"

#include "skopin/kiln.hpp"

#include <set>
#include <vector>

namespace skopin {

NightReport runNightFire(const std::vector<Ware>& queue) {
    NightReport report;

    
    std::set<std::string> seen;
    std::vector<Ware> waiting;

    for (const Ware& ware : queue) {
        if (!seen.insert(ware.mark).second) {
            report.refused.push_back({ware.mark, "duplicate"});
            continue;
        }

        if (validateWare(ware)) {
            report.refused.push_back({ware.mark, "invalid"});
            continue;
        }

        if (ware.thickness_mm > kCrackThicknessMm) {
            report.refused.push_back({ware.mark, "thick"});
            continue;
        }

        waiting.push_back(ware);
    }

    Kiln kiln;

    while (!waiting.empty()) {
        std::vector<Ware> batch;
        std::vector<bool> selected(waiting.size(), false);

        
        const Program program = waiting.front().program;
        batch.push_back(waiting.front());
        selected[0] = true;

        
        if (waiting.front().kind != WareKind::Sagger) {
            bool skipped = false;

            for (std::size_t i = 1; i < waiting.size(); ++i) {
                const Ware& candidate = waiting[i];

                
                if (candidate.program != program ||
                    candidate.kind == WareKind::Sagger) {
                    break;
                }

                const bool fits = fitsInKiln(batch, candidate);

                if (fits && (!skipped || candidate.kind == WareKind::Rush)) {
                    batch.push_back(candidate);
                    selected[i] = true;
                } else {
                    skipped = true;
                }
            }
        }

        
        std::vector<Ware> remaining;

        for (std::size_t i = 0; i < waiting.size(); ++i) {
            if (!selected[i]) {
                remaining.push_back(waiting[i]);
            }
        }

        waiting = remaining;

        const bool nextIsGlaze =
            !waiting.empty() && waiting.front().program == Program::Glaze;

        
        if (kiln.door() == Door::Closed) {
            kiln.open();
        }

        for (std::size_t i = batch.size(); i > 0; --i) {
            kiln.load(batch[i - 1]);
        }

        kiln.close();

        if (program == Program::Bisque) {
            
            kiln.fire();

            if (kiln.door() == Door::Closed) {
                kiln.open();
            }

            
            for (const Ware& ware : batch) {
                kiln.unload(ware.mark);
                report.passed.push_back(ware.mark);
            }

            
            if (!nextIsGlaze) {
                kiln.close();
                kiln.cool();
            }
        } else {
            
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