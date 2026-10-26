#include "modmenu/internal/Settings.hpp"

#include "modmenu/internal/Registry.hpp"

#include <unordered_map>

namespace modmenu::detail {
namespace {

std::unordered_map<std::string, std::vector<Setting>> settingsCache;
uint64_t revision = 0;

}

const std::vector<Setting>& SettingsOf(const ModInfo& mod) {
    auto [it, inserted] = settingsCache.try_emplace(mod.id);
    if (inserted) it->second = mod.settings ? mod.settings() : std::vector<Setting>{};
    return it->second;
}

void InvalidateSettings(const std::string& modId) {
    settingsCache.erase(modId);
    ++revision;
}

uint64_t SettingsRevision() { return revision; }

void ResetAll(size_t modIndex) {
    if (modIndex < Registry().size())
        for (const auto& row : SettingsOf(Registry()[modIndex])) row.Reset();
}

}
