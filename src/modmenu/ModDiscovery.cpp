#include "modmenu/ModMenu.hpp"

#include "ll/api/io/FileUtils.h"
#include "ll/api/mod/Manifest.h"
#include "ll/api/mod/Mod.h"
#include "ll/api/mod/ModManagerRegistry.h"
#include "ll/api/mod/NativeMod.h"

#include "modmenu/internal/Registry.hpp"
#include "modmenu/internal/ModMenuSettings.hpp"
#include "nlohmann/json.hpp"
#include "nugget/Log.hpp"

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

namespace nugget::modmenu {
namespace {

using detail::NormName;
using detail::Registry;
using detail::ResolveModIcon;

constexpr const char* NO_DESC = "No description provided.";
std::set<std::string> discoveredIds;

using Extra = std::map<std::string, std::string>;


Extra ExtraFromJson(const nlohmann::json& manifest) {
    Extra out;
    for (const char* key : {"extraInfo", "extrainfo"}) {
        const auto it = manifest.find(key);
        if (it == manifest.end() || !it->is_object()) continue;
        for (const auto& [k, v] : it->items())
            if (v.is_string()) out[k] = v.get<std::string>();
        break;
    }
    return out;
}


Extra ExtraFromMod(const ll::mod::Manifest& mf) {
    Extra out;
    if (mf.extraInfo)
        for (const auto& [k, v] : *mf.extraInfo) out[k] = v;
    return out;
}


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

int DiscoverLoadedMods() {
    int added = 0;
    for (ll::mod::Mod& mod : ll::mod::ModManagerRegistry::getInstance().mods()) {
        try {
            const auto& mf = mod.getManifest();
            if (mf.name.empty()) continue;
            const std::string key = NormName(mf.name);

            if (ModInfo* have = FindByName(key)) {
                if (!discoveredIds.count(have->id)) {
                    ApplyExtra(*have, ExtraFromMod(mf), mod.getModDir());
                    continue;
                }
            }

            ModInfo info;
            info.id = key;
            info.name = mf.name;
            info.version = mf.version ? mf.version->to_string() : std::string("unknown");
            info.author = mf.author.value_or("");
            info.description = mf.description.value_or(NO_DESC);

            info.badges.clear();
            if (mf.type == "preload-native") info.badges.push_back("Library");
            else if (mf.platform && !mf.platform->empty()) {
                std::string p = *mf.platform;
                p[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(p[0])));
                info.badges.push_back(std::move(p));
            } else info.badges.push_back("Mod");
            if (!mod.isEnabled()) info.badges.push_back("Disabled");

            ApplyExtra(info, ExtraFromMod(mf), mod.getModDir());

            if (discoveredIds.insert(info.id).second) ++added;
            RegisterMod(std::move(info));
        } catch (const std::exception& e) {
            nugget::Log().error("modmenu: skipped a mod: {}", e.what());
        } catch (...) {
            nugget::Log().error("modmenu: skipped a mod");
        }
    }
    return added;
}


int DiscoverUnloadedMods() {
    int added = 0;
    std::error_code ec;
    const auto dir = ll::mod::NativeMod::current()->getModDir().parent_path();
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        try {
            const auto text = ll::file_utils::readFile(entry.path() / "manifest.json");
            if (!text) continue;
            const auto j = nlohmann::json::parse(*text, nullptr, false);
            const std::string name = j.is_object() ? j.value("name", std::string()) : std::string();
            if (name.empty()) continue;
            const std::string key = NormName(name);
            if (FindByName(key)) continue;

            ModInfo info;
            info.id = key;
            info.name = name;
            const std::string version = j.value("version", std::string());
            const std::string description = j.value("description", std::string());
            info.version = version.empty() ? "unknown" : version;
            info.author = j.value("author", std::string());
            info.description = description.empty() ? NO_DESC : description;
            ApplyExtra(info, ExtraFromJson(j), entry.path());
            info.badges = {"Not loaded"};
            discoveredIds.insert(info.id);
            RegisterMod(std::move(info));
            ++added;
        } catch (...) {
        }
    }
    return added;
}

}

void RegisterMod(ModInfo info) {
    detail::InvalidateSettings(info.id);
    auto& mods = detail::Registry();
    auto it = std::find_if(mods.begin(), mods.end(), [&](const ModInfo& m) { return m.id == info.id; });
    if (it != mods.end()) *it = std::move(info);
    else mods.push_back(std::move(info));
    std::stable_sort(mods.begin(), mods.end(), [](const ModInfo& a, const ModInfo& b) { return a.name < b.name; });
}

void DiscoverInstalledMods() {
    int added = 0;
    try { added += DiscoverLoadedMods(); } catch (...) { nugget::Log().error("modmenu: mod discovery failed"); }
    added += DiscoverUnloadedMods();
    if (added) nugget::Log().info("modmenu: listed {} installed mod(s)", added);
}

}
