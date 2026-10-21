#include "modmenu/internal/ModMenuSettings.hpp"

#include "modmenu/internal/Registry.hpp"
#include "nugget/Keybinds.hpp"
#include "nugget/Settings.hpp"

#include "MossUI/core/Keys.hpp"

#include <unordered_map>

namespace nugget::modmenu::detail {

using MossUI::BaseKey;
using MossUI::Key;
using MossUI::KeyCode;

namespace {

std::unordered_map<std::string, std::vector<Setting>> settingsCache;
std::unordered_map<std::string, uint64_t> settingsRevisions;
uint64_t allSettingsRevision = 0;

}

std::string QuickAction(const std::string& id) { return "quick." + id; }

Setting QuickRow(const ModInfo& mod) {
    const std::string action = QuickAction(mod.id);
    const int def = mod.quickConfigKey;
    return Setting::KeyBind(
               "Quick config key (in game)",
               [action, def] { return keybinds::Get(action, def); },
               [action](int code) { keybinds::Set(action, code); },
               BaseKey(def))
        .Tip("Opens this mod's settings over the game. Also listed in Settings > Controls.")
        .ShownIf([] { return InGameEnabled(); });
}

const std::vector<Setting>& SettingsOf(const ModInfo& mod) {
    auto [it, inserted] = settingsCache.try_emplace(mod.id);
    if (inserted) {
        it->second = mod.settings ? mod.settings() : std::vector<Setting>{};
        if (mod.quickConfigKey) it->second.push_back(QuickRow(mod));
    }
    return it->second;
}

void InvalidateSettings(const std::string& modId) {
    settingsCache.erase(modId);
    ++settingsRevisions[modId];
    ++allSettingsRevision;
}

uint64_t SettingsRevision(const std::string& modId) { return settingsRevisions[modId]; }
uint64_t AllSettingsRevision() { return allSettingsRevision; }

}
