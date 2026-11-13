#include "skopin/night.hpp"
#include "skopin/measures.hpp"
#include "skopin/kiln.hpp"
#include <unordered_set>
#include <stdexcept>

namespace skopin {

NightReport runNightFire(const std::vector<Ware>& queue) {
    NightReport report;
    std::unordered_set<std::string> seen_marks;
    std::vector<Ware> valid_queue;

    for (const auto& ware : queue) {
        if (!seen_marks.insert(ware.mark).second) {
            report.refused.push_back(Refusal{ware.mark, "duplicate"});
            continue;
        }
        if (validateWare(ware).has_value()) {
            report.refused.push_back(Refusal{ware.mark, "invalid"});
            continue;
        }
        if (ware.thickness_mm > kCrackThicknessMm) {
            report.refused.push_back(Refusal{ware.mark, "thick"});
            continue;
        }
        valid_queue.push_back(ware);
    }

    std::vector<std::vector<Ware>> batches;
    std::vector<bool> taken(valid_queue.size(), false);
    std::size_t remaining = valid_queue.size();

    while (remaining > 0) {
        std::size_t first_idx = 0;
        while (taken[first_idx]) {
            ++first_idx;
        }

        std::vector<Ware> batch;
        batch.push_back(valid_queue[first_idx]);
        taken[first_idx] = true;
        --remaining;

        if (valid_queue[first_idx].kind != WareKind::Sagger) {
            const Program batch_program = valid_queue[first_idx].program;
            bool skipped_any = false;

            for (std::size_t i = first_idx + 1; i < valid_queue.size(); ++i) {
                if (taken[i]) {
                    continue;
                }
                const Ware& candidate = valid_queue[i];

                // Чужая программа или капсель — стена, просмотр останавливается.
                if (candidate.program != batch_program || candidate.kind == WareKind::Sagger) {
                    break;
                }

                const bool can_infiltrate = (!skipped_any || candidate.kind == WareKind::Rush);
                if (can_infiltrate && fitsInKiln(batch, candidate)) {
                    batch.push_back(candidate);
                    taken[i] = true;
                    --remaining;
                } else {
                    skipped_any = true;
                }
            }
        }

        batches.push_back(std::move(batch));
    }

    Kiln kiln;

    for (std::size_t b = 0; b < batches.size(); ++b) {
        const auto& batch = batches[b];
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
            for (const auto& ware : batch) {
                kiln.unload(ware.mark);
                report.passed.push_back(ware.mark);
            }

            const bool next_is_glaze = (b + 1 < batches.size() &&
                                        batches[b + 1].front().program == Program::Glaze);
            if (!next_is_glaze) {
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
            for (const auto& ware : batch) {
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