#include "modmenu/internal/Registry.hpp"

#include "modmenu/internal/Settings.hpp"
#include "platform/Log.hpp"

#include <algorithm>
#include <cctype>
#include <system_error>

namespace modmenu::detail {

std::string NormName(const std::string& name) {
    std::string out;
    for (unsigned char c : name) {
        if (std::isalnum(c)) out.push_back(static_cast<char>(std::tolower(c)));
    }
    return out;
}

std::vector<ModInfo>& Registry() {
    static std::vector<ModInfo> mods;
    return mods;
}

int IndexOf(const std::string& id) {
    const auto& mods = Registry();
    for (size_t i = 0; i < mods.size(); ++i)
        if (mods[i].id == id) return static_cast<int>(i);
    return -1;
}

std::string SelfVersion() {
    const auto& version = Self().getManifest().version;
    return version ? version->to_string() : std::string("1.0.0");
}

std::string ResolveModIcon(const std::filesystem::path& modDir, const std::string& icon) {
    if (icon.starts_with("textures/")) return icon;
    const std::filesystem::path relative(icon);
    if (relative.empty() || relative.is_absolute() || relative.has_root_name() || relative.has_parent_path() ||
        relative.extension() != ".png")
        return {};

    std::error_code ec;
    const auto file = modDir / relative;
    if (!std::filesystem::is_regular_file(file, ec) || ec) return {};
    const auto canonicalDir = std::filesystem::weakly_canonical(modDir, ec);
    if (ec) return {};
    const auto canonicalFile = std::filesystem::canonical(file, ec);
    if (ec || canonicalFile.parent_path() != canonicalDir) return {};
    return canonicalFile.generic_string();
}

}

namespace modmenu {

void RegisterMod(ModInfo info) {
    detail::InvalidateSettings(info.id);
    auto& mods = detail::Registry();
    auto it = std::find_if(mods.begin(), mods.end(), [&](const ModInfo& m) { return m.id == info.id; });
    if (it != mods.end()) *it = std::move(info);
    else mods.push_back(std::move(info));
    std::stable_sort(mods.begin(), mods.end(), [](const ModInfo& a, const ModInfo& b) { return a.name < b.name; });
}

}
