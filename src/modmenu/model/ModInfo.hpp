#pragma once

#include "modmenu/model/Setting.hpp"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace modmenu {

inline constexpr const char* BADGE_CLIENT = "Client";
inline constexpr const char* BADGE_MOD = "Mod";
inline constexpr const char* BADGE_LIBRARY = "Library";
inline constexpr const char* BADGE_DISABLED = "Disabled";
inline constexpr const char* BADGE_NOT_LOADED = "Not loaded";

struct ModInfo {
    std::string id;
    std::string name;
    std::string description;
    std::string version = "1.0.0";
    std::string author;
    std::string icon;
    std::function<std::vector<Setting>()> settings;
    std::function<void()> openScreen;
    std::string website;
    std::string issues;
    std::string source;
    std::string license;
    std::vector<std::pair<std::string, std::string>> links;
    std::vector<std::string> badges = {BADGE_CLIENT};
};

}
