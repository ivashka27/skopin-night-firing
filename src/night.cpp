#include "skopin/night.hpp"

#include "skopin/kiln.hpp"

#include <stdexcept>

namespace skopin {

NightReport runNightFire(const std::vector<Ware>& queue) {
    NightReport report;
    std::vector<Ware> valid;
    std::vector<std::string> marks;

    Kiln kiln;

    for (const Ware& ware : queue) {
        bool duplicate = false;

        for (const std::string& mark : marks){
            if (mark == ware.mark) {
                duplicate = true;
                break;
            }
        }
        marks.push_back(ware.mark);

        if (duplicate == true) {
            report.refused.push_back({ware.mark, "duplicate"});
            continue;
        }
        std::optional<std::string> error = validateWare(ware);
        
        if (error.has_value()) {
            report.refused.push_back({ware.mark, "invalid"});
            continue;
        }
        if (ware.thickness_mm > kCrackThicknessMm) {
            report.refused.push_back({ware.mark, "thick"});
            continue;
        }
        valid.push_back(ware);
    }


    while (!valid.empty()) {
        std::vector<Ware> batch;
        std::vector<Ware> remaining;

        Ware first = valid[0];
        batch.push_back(first);

        if (first.kind == WareKind::Sagger) {
            for (std::size_t i = 1; i < valid.size(); i++) {
                remaining.push_back(valid[i]);
            }
        }
        else {
            bool skip = false;
            for (std::size_t j = 1; j < valid.size(); j++) {
                const Ware& current = valid[j];

                if (current.program != first.program || current.kind == WareKind::Sagger) {
                    for (std::size_t k = j; k < valid.size(); k++) {
                        remaining.push_back(valid[k]);
                    }

                    break;
                }

                bool fits = fitsInKiln(batch, current);

                if (fits && (!skip || current.kind == WareKind::Rush)) {
                    batch.push_back(current);
                }
                else {
                    skip = true;
                    remaining.push_back(current);
                }
            }
        }
        valid = remaining;

        bool hasNextBatch = !valid.empty();
        bool nextIsGlaze = false;

        if (hasNextBatch) {
            nextIsGlaze = valid[0].program == Program::Glaze;
        }

        if (first.program == Program::Bisque) {
            if (kiln.door() == Door::Open) {
                kiln.close();
            }

            while (kiln.cone() != Cone::Cold) {
                kiln.cool();
            }

            kiln.open();

            for (std::size_t i = batch.size(); i > 0; i--) {
                kiln.load(batch[i - 1]);
            }

            kiln.close();
            kiln.fire();
            kiln.open();

            for (const Ware& ware : batch) {
                kiln.unload(ware.mark);
                report.passed.push_back(ware.mark);
            }

            if (!nextIsGlaze) {
                kiln.close();
                kiln.cool();
            }
        }
        else if (first.program == Program::Glaze) {

            if (kiln.door() == Door::Closed) {
                kiln.open();
            }

            for (std::size_t i = batch.size(); i > 0; i--) {
                kiln.load(batch[i - 1]);
            }

            kiln.close();

            if (kiln.cone() == Cone::Cold) {
                kiln.fire();
                kiln.fire();
            }
            else if (kiln.cone() == Cone::Bisque) {
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
