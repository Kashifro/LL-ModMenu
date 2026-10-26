#include "modmenu/ModMenu.hpp"

#include "ll/api/io/FileUtils.h"
#include "ll/api/mod/Mod.h"
#include "ll/api/mod/ModManagerRegistry.h"
#include "ll/api/mod/NativeMod.h"

#include "modmenu/internal/JsonConfig.hpp"
#include "modmenu/internal/Registry.hpp"
#include "modmenu/internal/Settings.hpp"
#include "nlohmann/json.hpp"
#include "platform/Log.hpp"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <exception>
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>

namespace modmenu {
namespace {

using json = nlohmann::json;
using detail::NormName;
using detail::Registry;
using detail::ResolveModIcon;
using Extra = std::map<std::string, std::string>;

constexpr const char* NO_DESC = "No description provided.";
std::set<std::string> discoveredIds;

std::optional<json> ReadManifest(const std::filesystem::path& modDir) {
    const auto text = ll::file_utils::readFile(modDir / "manifest.json");
    if (!text) return std::nullopt;
    auto j = json::parse(*text, nullptr, false);
    if (!j.is_object() || j.value("name", std::string()).empty()) return std::nullopt;
    return j;
}

Extra ExtraFrom(const json& manifest) {
    Extra out;
    const auto it = manifest.find("extraInfo");
    if (it != manifest.end() && it->is_object())
        for (const auto& [k, v] : it->items())
            if (v.is_string()) out[k] = v.get<std::string>();
    return out;
}

// Fills fields the manifest left empty from its "extraInfo" block.
void ApplyExtra(ModInfo& info, const Extra& extra, const std::filesystem::path& modDir) {
    auto get = [&](const char* key) {
        const auto it = extra.find(key);
        return it == extra.end() ? std::string() : it->second;
    };
    auto assign = [&](std::string& field, const char* key) {
        if (field.empty())
            if (const std::string value = get(key); !value.empty()) field = value;
    };

    assign(info.author, "author");
    if (info.description.empty() || info.description == NO_DESC)
        if (const std::string desc = get("description"); !desc.empty()) info.description = desc;
    assign(info.website, "website");
    assign(info.issues, "issues");
    assign(info.source, "source");
    assign(info.license, "license");
    if (info.icon.empty())
        if (const std::string icon = get("icon"); !icon.empty())
            info.icon = ResolveModIcon(modDir, icon);

    for (const auto& [key, label] : std::array<std::pair<const char*, const char*>, 4>{
             std::pair{"wiki", "Wiki"}, {"discord", "Discord"}, {"support", "Support"}, {"donate", "Donate"}}) {
        const std::string url = get(key);
        if (!url.empty() && std::none_of(info.links.begin(), info.links.end(),
                                         [&](const auto& current) { return current.first == label; }))
            info.links.emplace_back(label, url);
    }
}

ModInfo* FindByName(const std::string& key) {
    for (auto& have : Registry())
        if (NormName(have.id) == key || NormName(have.name) == key) return &have;
    return nullptr;
}

ModInfo InfoFrom(const json& manifest, const std::filesystem::path& modDir) {
    ModInfo info;
    info.name = manifest.value("name", std::string());
    info.id = NormName(info.name);
    const std::string version = manifest.value("version", std::string());
    const std::string description = manifest.value("description", std::string());
    info.version = version.empty() ? "unknown" : version;
    info.author = manifest.value("author", std::string());
    info.description = description.empty() ? NO_DESC : description;
    ApplyExtra(info, ExtraFrom(manifest), modDir);
    return info;
}

std::string Capitalized(std::string s) {
    s[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[0])));
    return s;
}

int DiscoverLoadedMods() {
    int added = 0;
    for (ll::mod::Mod& mod : ll::mod::ModManagerRegistry::getInstance().mods()) {
        try {
            const auto manifest = ReadManifest(mod.getModDir());
            if (!manifest) continue;
            ModInfo info = InfoFrom(*manifest, mod.getModDir());

            if (ModInfo* have = FindByName(info.id)) {
                if (!discoveredIds.erase(have->id)) {
                    ApplyExtra(*have, ExtraFrom(*manifest), mod.getModDir());
                    continue;
                }
            }

            const std::string type = manifest->value("type", std::string());
            const std::string platform = manifest->value("platform", std::string());
            info.badges.clear();
            if (type == "preload-native") info.badges.push_back(BADGE_LIBRARY);
            else info.badges.push_back(platform.empty() ? BADGE_MOD : Capitalized(platform));
            if (!mod.isEnabled()) info.badges.push_back(BADGE_DISABLED);

            detail::Notify notify;
            if (type == "native" || type == "preload-native") {
                const auto dll = static_cast<HMODULE>(static_cast<ll::mod::NativeMod&>(mod).getHandle());
                if (auto fn = reinterpret_cast<void (*)()>(GetProcAddress(dll, "ModMenu_OpenConfigScreen")))
                    info.openScreen = [fn] { fn(); };
                if (auto fn = reinterpret_cast<void (*)(const char*)>(GetProcAddress(dll, "ModMenu_OnConfigChanged")))
                    notify = [fn](const std::string& path) { fn(path.c_str()); };
            }
            // Mods that registered nothing still get a page, built from their own config.json.
            if (const auto cfg = mod.getConfigDir() / "config.json"; std::filesystem::exists(cfg))
                info.settings = [cfg, notify] { return detail::JsonConfigSettings(cfg, notify); };

            if (discoveredIds.insert(info.id).second) ++added;
            RegisterMod(std::move(info));
        } catch (const std::exception& e) {
            Log().error("modmenu: skipped a mod: {}", e.what());
        } catch (...) {
            Log().error("modmenu: skipped a mod");
        }
    }
    return added;
}

int DiscoverUnloadedMods() {
    int added = 0;
    std::error_code ec;
    const auto dir = Self().getModDir().parent_path();
    std::set<std::string> found;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        try {
            const auto manifest = ReadManifest(entry.path());
            if (!manifest) continue;
            ModInfo info = InfoFrom(*manifest, entry.path());
            if (FindByName(info.id)) {
                if (discoveredIds.count(info.id)) found.insert(info.id);
                continue;
            }
            info.badges = {BADGE_NOT_LOADED};
            discoveredIds.insert(info.id);
            found.insert(info.id);
            RegisterMod(std::move(info));
            ++added;
        } catch (...) {
            Log().warn("modmenu: skipped unreadable manifest in {}", entry.path().string());
        }
    }
    if (ec) {
        Log().error("modmenu: could not scan mod directory: {}", ec.message());
        return added;
    }
    std::erase_if(Registry(), [&](const ModInfo& info) {
        return discoveredIds.contains(info.id) && !found.contains(info.id);
    });
    discoveredIds = std::move(found);
    return added;
}

}

void DiscoverInstalledMods() {
    int added = 0;
    try { added += DiscoverLoadedMods(); } catch (...) { Log().error("modmenu: mod discovery failed"); }
    added += DiscoverUnloadedMods();
    if (added) Log().info("modmenu: listed {} installed mod(s)", added);
}

}
