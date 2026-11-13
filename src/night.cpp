#include "skopin/night.hpp"
#include <string>
#include "skopin/kiln.hpp"
#include "skopin/ware.hpp"
#include <vector>
#include <stdexcept>

namespace skopin {

NightReport runNightFire(const std::vector<Ware>& queue) {
    NightReport report;
    std::vector<std::string> all_statics;
    std::vector<Ware> accept_statics;
    for (int i = 0; i < (int)queue.size(); i++) {
        Ware current_state = queue[i];
        bool same = false;
        for (int j = 0; j < (int)all_statics.size(); j++) {
            if (all_statics[j] == current_state.mark) {
                same = true;
                break;
            }
        }
        if (same == true) {
            report.refused.push_back({current_state.mark, "duplicate"});
            continue;
        } else {
            all_statics.push_back(current_state.mark);
        }
        if (validateWare(current_state).has_value() == true) {
            report.refused.push_back({current_state.mark, "invalid"});
            continue;
        }
        if (current_state.thickness_mm > 12) {
            report.refused.push_back({current_state.mark, "thick"}); 
            continue;
        }
        accept_statics.push_back(current_state);
    }
    Kiln kiln;
    while (accept_statics.size() > 0) {
        std::vector<Ware> current_queue;
        std::vector<Ware> next_queue;
        if (accept_statics[0].kind == WareKind::Sagger) {
            current_queue.push_back(accept_statics[0]);
            for (int i = 1; i < (int)accept_statics.size(); i++) { 
                next_queue.push_back(accept_statics[i]);
            }
        } else {
            Program current_prog = accept_statics[0].program; 
            bool flag = false;
            bool wall = false;

            for (int i = 0; i < (int)accept_statics.size(); i++) {
                Ware state = accept_statics[i];
                if (wall == true) {
                    next_queue.push_back(state);
                    continue;
                }
                if (i > 0 && (state.program != current_prog || state.kind == WareKind::Sagger)) {
                    wall = true;
                    next_queue.push_back(state);
                    continue;
                }
                if (i == 0) {
                    current_queue.push_back(state);
                    continue;
                }

                bool fits = fitsInKiln(current_queue, state); 
                if (fits == true) {
                    if (flag == false || state.kind == WareKind::Rush) {
                        current_queue.push_back(state);
                    } else {
                        next_queue.push_back(state);
                    }
                } else {
                    next_queue.push_back(state);
                    if (state.kind != WareKind::Rush) {
                        flag = true;
                    }
                }
            }
        }
        accept_statics = next_queue;
        Program now_prog = current_queue[0].program;
        if (now_prog == Program::Bisque) {
            if (kiln.door() == Door::Open) {
                kiln.close();
            }
            if (kiln.cone() == Cone::Bisque) {
                kiln.cool();
            }
            
            kiln.open();
            for (int i = (int)current_queue.size() - 1; i >= 0; i--) {
                kiln.load(current_queue[i]);
            }
            kiln.close();
            kiln.fire();
            
            kiln.open();
            for (int i = 0; i < (int)current_queue.size(); i++) {
                kiln.unload(current_queue[i].mark);
            }
            bool glaze_ = false;
            if (accept_statics.size() > 0 && accept_statics[0].program == Program::Glaze) {
                glaze_ = true;
            }

            if (glaze_ == false) {
                kiln.close();
                kiln.cool();
            }
            
        } else {
            if (kiln.door() == Door::Closed) {
                kiln.open();
            }
            for (int i = (int)current_queue.size() - 1; i >= 0; i--) {
                kiln.load(current_queue[i]);
            }
            kiln.close();

            if (kiln.cone() == Cone::Cold) {
                kiln.fire();
                kiln.fire();
            } else if (kiln.cone() == Cone::Bisque) {
                kiln.fire();
            }
            kiln.cool();
            kiln.cool();
            kiln.open();
            for (int i = 0; i < (int)current_queue.size(); i++) {
                kiln.unload(current_queue[i].mark);
            }
            kiln.close();
        }
        for (int i = 0; i < (int)current_queue.size(); i++) {
            report.passed.push_back(current_queue[i].mark);
        }
    }
    report.stokes = kiln.stokes();
    return report;
}
} // namespace skopin