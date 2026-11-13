#include "skopin/night.hpp"

#include "skopin/kiln.hpp"

#include <stdexcept>

namespace skopin {

NightReport runNightFire(const std::vector<Ware>& queue) {
    NightReport report;
    std::vector<Ware> waiting;
    int n = queue.size()
    for (int i = 0; i < n; i++) {
        const Ware& ware = queue[i];
        bool duplicate = false;
        for (int j = 0; j < i; j++) {
            if (queue[j].mark == ware.mark) {
                duplicate = true;
            }
        }

        std::string reason;
        if (duplicate) {
            reason = "duplicate";
        } else if (validateWare(ware).has_value()) {
            reason = "invalid";
        } else if (ware.thickness_mm > kCrackThicknessMm) {
            reason = "thick";
        }

        if (reason == "") {
            waiting.push_back(ware);
        } else {
            Refusal refusal;
            refusal.mark = ware.mark;
            refusal.reason = reason;
            report.refused.push_back(refusal);
        }
    }

    Kiln kiln;
    while (!waiting.empty()) {
        std::vector<Ware> batch;
        std::vector<Ware> remaining;
        batch.push_back(waiting[0]);
        Program program = waiting[0].program;
        bool wall = false;
        bool skipped = false;
        if (waiting[0].kind == WareKind::Sagger) {
            wall = true;
        }

        int n = waiting.size()
        for (int i = 1; i < n; i++) {
            const Ware& ware = waiting[i];
            if (ware.program != program || ware.kind == WareKind::Sagger) {
                wall = true;
            }
            if (wall) {
                remaining.push_back(ware);
            } else if (fitsInKiln(batch, ware) &&
                       (!skipped || ware.kind == WareKind::Rush)) {
                batch.push_back(ware);
            } else {
                skipped = true;
                remaining.push_back(ware);
            }
        }
        waiting = remaining;

        if (kiln.door() == Door::Closed) {
            kiln.open();
        }
        for (int i = batch.size(); i > 0; i--) {
            kiln.load(batch[i - 1]);
        }
        kiln.close();

        if (program == Program::Bisque) {
            kiln.fire();
        } else {
            if (kiln.cone() == Cone::Cold) {
                kiln.fire();
            }
            kiln.fire();
            kiln.cool();
            kiln.cool();
        }

        kiln.open();
        for (int i = 0; i < batch.size(); i++) {
            kiln.unload(batch[i].mark);
            report.passed.push_back(batch[i].mark);
        }

        bool hotNext = false;
        if (program == Program::Bisque && !waiting.empty()) {
            if (waiting[0].program == Program::Glaze) {
                hotNext = true;
            }
        }
        if (!hotNext) {
            kiln.close();
            if (program == Program::Bisque) {
                kiln.cool();
            }
        }
    }
    report.stokes = kiln.stokes();
    return report;
}

}  // namespace skopin
