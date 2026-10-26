#pragma once

#include "modmenu/ModMenu.hpp"

#include <cstdint>

namespace modmenu::detail {

inline constexpr const char* OPT_BADGES = "modmenu.badges";
inline constexpr const char* OPT_LIST_SUBTITLE = "modmenu.list_subtitle";
inline constexpr const char* OPT_COUNT = "modmenu.count";
inline constexpr const char* OPT_SHOW_LINKS = "modmenu.show_links";
inline constexpr const char* OPT_SHOW_LICENSE = "modmenu.show_license";
inline constexpr const char* OPT_SHOW_CREDITS = "modmenu.show_credits";
inline constexpr const char* OPT_TOOLTIPS = "modmenu.tooltips";
inline constexpr const char* OPT_TEXT_SCALE = "modmenu.text_scale";
inline constexpr const char* OPT_SHOW_DISABLED = "modmenu.show_disabled";
inline constexpr const char* OPT_SHOW_LIBRARIES = "modmenu.show_libraries";
inline constexpr const char* OPT_SHOW_UNLOADED = "modmenu.show_unloaded";
inline constexpr const char* OPT_SEARCH_DESC = "modmenu.search_desc";
inline constexpr const char* OPT_SEARCH_SETTINGS = "modmenu.search_settings";
inline constexpr const char* OPT_REMEMBER = "modmenu.remember";
inline constexpr const char* OPT_INGAME = "modmenu.ingame";
inline constexpr const char* OPT_SORT_DESC = "modmenu.sort_desc";
inline constexpr const char* OPT_SHOW = "modmenu.show";
inline constexpr const char* OPT_LAST = "modmenu.last";
inline constexpr const char* OPT_DIM = "modmenu.dim";
inline constexpr const char* OPT_DIM_INGAME = "modmenu.dim_ingame";
inline constexpr const char* OPT_DIRTBG = "modmenu.dirtbg";
inline constexpr const char* OPT_DIRTBG_INGAME = "modmenu.dirtbg_ingame";
inline constexpr const char* OPT_CLICK = "modmenu.click";
inline constexpr const char* OPT_CLICK_VOLUME = "modmenu.click_volume";
inline constexpr const char* OPEN_KEY_NAME = "open";

const std::vector<Setting>& SettingsOf(const ModInfo& mod);
void InvalidateSettings(const std::string& modId);
uint64_t SettingsRevision();
void ResetAll(size_t modIndex);
int ListSubtitleMode();
std::vector<Setting> BuildModMenuSettings();

}
