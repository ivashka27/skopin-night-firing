#include "skopin/night.hpp"

#include "skopin/kiln.hpp"

#include <stdexcept>

namespace skopin {

NightReport runNightFire(const std::vector<Ware>& wares) {
    NightReport report;
    std::vector<Ware> validWare;
    std::vector<std::string> seenMarks;

    for (auto ware : wares) {
        
        bool duplicate = false;
        for (auto mark : seenMarks) {
            if (ware.mark == mark) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) {
            report.refused.push_back({ ware.mark, "duplicate" });
            continue;
        }
        seenMarks.push_back(ware.mark);
        
        if (validateWare(ware) != std::nullopt) {
            report.refused.push_back({ ware.mark, "invalid" });
            continue;
        }
        
        if (ware.thickness_mm > kCrackThicknessMm) {
            report.refused.push_back({ ware.mark, "thick" });
            continue;
        }

        validWare.push_back(ware);
    }

    if (validWare.empty()) {
        return report;
    }

    std::vector<std::vector<Ware>> batches;
    std::vector<Ware> remaining = validWare;

    while (!remaining.empty()) {
        Ware base = remaining.front();
        Program program = base.program;

        std::vector<Ware> batch;
        std::vector<int> selectedIndexes;

        batch.push_back(base);
        selectedIndexes.push_back(0);

        if (base.kind != WareKind::Sagger) {
            bool skippedSomeone = false;
            for (int i = 1; i < remaining.size(); i++) {
                Ware candidate = remaining[i];
                if (candidate.program != program || candidate.kind == WareKind::Sagger) {
                    break;
                }
                if (fitsInKiln(batch, candidate)) {
                    if (!skippedSomeone || candidate.kind == WareKind::Rush) {
                        batch.push_back(candidate);
                        selectedIndexes.push_back(i);
                    }
                    else {
                        skippedSomeone = true;
                    }
                }
                else {
                    skippedSomeone = true;
                }
            }
        }
        for (int i = static_cast<int>(selectedIndexes.size()) - 1; i >= 0; --i) {
            remaining.erase(remaining.begin() + selectedIndexes[i]);
        }
        batches.push_back(batch);
    }

    Kiln kiln;

    for (int b = 0; b < batches.size(); b++) {
        auto batch = batches[b];
        bool nextIsGlaze = false;
        if (b + 1 < batches.size()) {
            if (batches[b + 1][0].program == Program::Glaze) {
                nextIsGlaze = true;
            }
        }
        if (batch[0].program == Program::Bisque) {
            if (kiln.door() == Door::Open) {
                kiln.close();
            }
            if (kiln.cone() == Cone::Bisque) {
                kiln.cool();
            }
            kiln.open();

            for (int i = batch.size() - 1; i >= 0; i--) {
                kiln.load(batch[i]);
            }

            kiln.close();
            kiln.fire();
            kiln.open();

            for (auto arg : batch) {
                kiln.unload(arg.mark);
                report.passed.push_back(arg.mark);
            }

            if (!nextIsGlaze) {
                kiln.close();
                kiln.cool();
            }
        }
        else {
            if (kiln.door() == Door::Closed) {
                kiln.open();
            }

            for (int i = batch.size() - 1; i >= 0; i--) {
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

            for (auto arg : batch) {
                kiln.unload(arg.mark);
                report.passed.push_back(arg.mark);
            }
            kiln.close();
        }
    }
    report.stokes = kiln.stokes();
    return report;
}

}  // namespace skopin
