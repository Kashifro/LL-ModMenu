#include "modmenu/internal/ModMenuSettings.hpp"

#include "modmenu/internal/Registry.hpp"
#include "nugget/Keybinds.hpp"

namespace nugget::modmenu::detail {
namespace {

std::string FindKey(const std::vector<Setting>& rows, int code, bool sameMod, const std::string& self) {
    for (const auto& row : rows) {
        if (row.kind == Setting::Kind::Category) {
            auto found = FindKey(row.children, code, sameMod, self);
            if (!found.empty()) return found;
            continue;
        }
        if ((sameMod && row.label == self) || row.kind != Setting::Kind::Key || !row.getKey) continue;
        if (row.getKey() == code) return row.label;
    }
    return {};
}

}

std::string KeyConflict(int code, size_t modIndex, const std::string& self, const SettingRows& settings) {
    if (code == 0) return {};
    for (size_t mod = 0; mod < settings.size(); ++mod) {
        const auto found = FindKey(settings[mod], code, mod == modIndex, self);
        if (!found.empty()) return Registry()[mod].name + ": " + found;
    }
    return keybinds::Conflict(code);
}

}
