#include "skopin/night.hpp"
#include <set>
#include "skopin/kiln.hpp"

#include <stdexcept>

namespace skopin {

NightReport runNightFire(const std::vector<Ware>& queue) {
    NightReport report;
    Kiln kiln;

    std::set<std::string> seen;
    std::vector<Ware> waiting;
    for (const Ware& ware : queue) {
        if (seen.count(ware.mark) > 0) {
            report.refused.push_back({ware.mark, "duplicate"});
            continue;
        }

        seen.insert(ware.mark);

        if (validateWare(ware).has_value()) {
            report.refused.push_back({ware.mark, "invalid"});
            continue;
        }
        if (ware.thickness_mm > kCrackThicknessMm){
            report.refused.push_back({ware.mark, "thick"});
            continue;
        }
        else{
            waiting.push_back(ware);
        }
    }
    std::vector<std::vector<Ware>> batches;

    while (waiting.size() > 0){
        std::vector<Ware> batch;
        std::vector<Ware> remaining;
        batch.push_back(waiting[0]);
        bool skip = false;
        bool wall = false;

        for (std::size_t i = 1; i < waiting.size(); i++){
            const Ware& ware = waiting[i];
            if (batch[0].kind == WareKind::Sagger || wall){
                remaining.push_back(ware);
                continue;
            }
            if (ware.program != batch[0].program || ware.kind == WareKind::Sagger){
                wall = true;
                remaining.push_back(ware);
                continue;
            }
            if (fitsInKiln(batch, ware) && (!skip || ware.kind == WareKind::Rush)){
                batch.push_back(ware);
            }
            else{
                skip = true;
                remaining.push_back(ware);
            }
        }
        batches.push_back(batch);
        waiting = remaining;
    }

    for (std::size_t i = 0; i < batches.size(); i++){
        const std::vector<Ware>& batch = batches[i];

        if (batch[0].program == Program::Bisque){
            if (kiln.door() == Door::Open){
                kiln.close();
            }
            if (kiln.cone() == Cone::Bisque){
                kiln.cool();
            }
            kiln.open();
            for (std::size_t j = batch.size(); j > 0; j--){
                kiln.load(batch[j - 1]);
            }
            kiln.close();
            kiln.fire();
            kiln.open();
            for (const Ware& ware : batch){
                kiln.unload(ware.mark);
                report.passed.push_back(ware.mark);
            }
            if (i + 1 < batches.size() && batches[i + 1][0].program == Program::Glaze){
                continue;
            }
            kiln.close();
            kiln.cool();
        }
        else{
            if (kiln.door() == Door::Closed){
                kiln.open();
            }
            for (std::size_t j = batch.size(); j > 0; j--){
                kiln.load(batch[j - 1]);
            }
            kiln.close();
            if (kiln.cone() == Cone::Cold){
                kiln.fire();
            }
            kiln.fire();
            kiln.cool();
            kiln.cool();
            kiln.open();
            for (const Ware& ware : batch){
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
