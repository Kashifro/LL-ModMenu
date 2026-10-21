#pragma once

#include "modmenu/ModMenu.hpp"

#include <cstdint>

namespace nugget::modmenu::detail {

inline constexpr const char* OPT_BADGES = "modmenu.badges";
inline constexpr const char* OPT_SUMMARY = "modmenu.summary";
inline constexpr const char* OPT_COMPACT = "modmenu.compact_list";
inline constexpr const char* OPT_LIST_SUBTITLE = "modmenu.list_subtitle";
inline constexpr const char* OPT_COUNT = "modmenu.count";
inline constexpr const char* OPT_LINKS = "modmenu.show_links";
inline constexpr const char* OPT_LICENSE = "modmenu.show_license";
inline constexpr const char* OPT_CREDITS = "modmenu.show_credits";
inline constexpr const char* OPT_TOOLTIPS = "modmenu.tooltips";
inline constexpr const char* OPT_TEXT_SCALE = "modmenu.text_scale";
inline constexpr const char* OPT_SHOW_OFF = "modmenu.show_disabled";
inline constexpr const char* OPT_SHOW_LIBS = "modmenu.show_libraries";
inline constexpr const char* OPT_SHOW_UNLOADED = "modmenu.show_unloaded";
inline constexpr const char* OPT_FIND_DESC = "modmenu.search_desc";
inline constexpr const char* OPT_FIND_ROWS = "modmenu.search_settings";
inline constexpr const char* OPT_REMEMBER = "modmenu.remember";
inline constexpr const char* OPT_INGAME = "modmenu.ingame";
inline constexpr const char* OPT_SORT_DESC = "modmenu.sort.desc";
inline constexpr const char* OPT_SHOW = "modmenu.show";
inline constexpr const char* OPT_LAST = "modmenu.last";
inline constexpr const char* OPT_SAVE_IMMEDIATE = "modmenu.save_immediate";
inline constexpr const char* OPT_DIM = "modmenu.dim";
inline constexpr const char* OPT_DIM_INGAME = "modmenu.dim_ingame";
inline constexpr const char* OPT_DIRT = "modmenu.dirtbg";
inline constexpr const char* OPT_DIRT_INGAME = "modmenu.dirtbg_ingame";
inline constexpr const char* OPEN_KEY_NAME = "open";

using SettingRows = std::vector<std::vector<Setting>>;

std::string QuickAction(const std::string& id);
Setting QuickRow(const ModInfo& mod);
const std::vector<Setting>& SettingsOf(const ModInfo& mod);
void InvalidateSettings(const std::string& modId);
uint64_t SettingsRevision(const std::string& modId);
uint64_t AllSettingsRevision();
void ResetAll(size_t modIndex);
std::string KeyConflict(int code, size_t modIndex, const std::string& self, const SettingRows& settings);
int ListSubtitleMode();
std::vector<Setting> BuildModMenuSettings();

}
