#include "modmenu/internal/ModMenuSettings.hpp"

#include "modmenu/internal/Registry.hpp"

#include "MossUI/core/Keys.hpp"
#include "ll/api/mod/NativeMod.h"
#include "nugget/Keybinds.hpp"
#include "nugget/Settings.hpp"

#include <cmath>
#include <utility>

namespace nugget::modmenu::detail {

using MossUI::Key;
using MossUI::KeyCode;

int ListSubtitleMode() {
    const int mode = settings::GetInt(OPT_LIST_SUBTITLE, -1);
    if (mode >= 0 && mode <= 2) return mode;
    if (settings::GetBool(OPT_COMPACT, false)) return 1;
    return settings::GetBool(OPT_SUMMARY, true) ? 0 : 2;
}

void ResetAll(size_t modIndex) {
    if (modIndex < Registry().size())
        for (const auto& row : SettingsOf(Registry()[modIndex])) row.Reset();
}

namespace {
auto flag(std::string label, const char* key, bool def, std::string tip) {
    return Setting::Toggle(std::move(label), [key, def] { return settings::GetBool(key, def); },
                           [key](bool value) { settings::SetBool(key, value); }, def)
        .Tip(std::move(tip));
}

auto percent(std::string label, const char* key, int def, std::string tip, float lo = 0, float hi = 100, float step = 1) {
    return Setting::Slider(std::move(label), [key, def] { return static_cast<float>(settings::GetInt(key, def)); },
                           [key](float value) { settings::SetInt(key, static_cast<int>(std::lround(value))); },
                           lo, hi, static_cast<float>(def), step, 0, "%")
        .Tip(std::move(tip));
}

}

std::vector<Setting> BuildModMenuSettings() {
    return {
        Setting::Category("Appearance", {
            Setting::Toggle("Write settings to disk immediately",
                            [] { return settings::GetBool(OPT_SAVE_IMMEDIATE, true); },
                            [](bool value) {
                                settings::SetBool(OPT_SAVE_IMMEDIATE, value);
                                settings::SetImmediateWrites(value);
                            }, true)
                .Tip("Write each changed setting to disk right away. Off saves when you close the menu or disable the mod."),
            flag("Dirt background", OPT_DIRT, true, "Tiled dirt behind the mod list and settings pages. Off = plain dim."),
            percent("Background darkness", OPT_DIM, 55, "How dark the backdrop is on the start screen."),
            percent("Text size", OPT_TEXT_SCALE, 100, "Scale MossUI text from 75% to 200%.", 75, 200, 5),
            flag("Badges", OPT_BADGES, true, "Client, Library, Disabled... tags next to mod names."),
            Setting::Choice("Mod list subtitle", {"Summary", "Version", "None"},
                            [] { return ListSubtitleMode(); },
                            [](int value) { settings::SetInt(OPT_LIST_SUBTITLE, value); }, 0)
                .Tip("Choose what appears below each mod name."),
            flag("Mod count", OPT_COUNT, true, "\"Showing N mods\" under the search box."),
            flag("Tooltips", OPT_TOOLTIPS, true, "Hover text on setting rows."),
        }),
        Setting::Category("Mod details", {
            flag("Show links", OPT_LINKS, true, "Show source and other metadata links."),
            flag("Show license", OPT_LICENSE, true, "Show the mod's license in its details."),
            flag("Show credits", OPT_CREDITS, true, "Show authors in the details pane and header."),
        }),
        Setting::Category("Mod list", {
            flag("Show disabled mods", OPT_SHOW_OFF, true, "Mods LeviLamina has switched off."),
            flag("Show libraries", OPT_SHOW_LIBS, true, "Preload libraries other mods depend on."),
            flag("Show mods that aren't loaded", OPT_SHOW_UNLOADED, true, "Folders in mods/ that LeviLamina didn't load."),
            flag("Search descriptions", OPT_FIND_DESC, true, "Search matches description text, not just names."),
            flag("Search setting names", OPT_FIND_ROWS, true, "Search also matches the names of a mod's settings."),
            flag("Remember last mod", OPT_REMEMBER, false, "Reopen the list on the mod you looked at last."),
        }),
        Setting::Category("Sound and keys", {
            flag("Click sounds", "modmenu.click", true, "Vanilla click on buttons, switches and list entries."),
            percent("Click volume", "modmenu.click_volume", 100, "Volume of the click."),
        }),
        Setting::Category("In game", {
            flag("Enable in game", OPT_INGAME, false,
                 "Lets LL-ModMenu and the mods' quick settings open over the game. Off hides the options below and disables their keys."),
            Setting::KeyBind("Open LL-ModMenu in game", [] { return keybinds::Get(OPEN_KEY_NAME, Key(KeyCode::M)); },
                             [](int code) { keybinds::Set(OPEN_KEY_NAME, code); }, Key(KeyCode::M))
                .Tip("Opens the mod list over the game. Also listed in Settings > Controls.")
                .ShownIf([] { return InGameEnabled(); }),
            flag("Dirt background in game", OPT_DIRT_INGAME, false, "Same for screens opened over the game. Off keeps the world visible.")
                .ShownIf([] { return InGameEnabled(); }),
            percent("Background darkness in game", OPT_DIM_INGAME, 30, "How dark the backdrop is over the game.")
                .ShownIf([] { return InGameEnabled(); }),
        }),
        Setting::Category("Data", {
            Setting::Section("Folders"),
            Setting::Action("Open data folder", [] { OpenFolder(ll::mod::NativeMod::current()->getDataDir()); })
                .Tip("Where settings.json lives."),
            Setting::Section("Mod list"),
            Setting::Action("Refresh mod list", [] { DiscoverInstalledMods(); })
                .Tip("Picks up mods loaded or installed since the menu opened."),
            Setting::Section("About"),
            Setting::Section("LL-ModMenu " + SelfVersion() + " - " + std::to_string(Registry().size()) + " mods listed"),
            Setting::Section("Reset"),
            Setting::Action("Reset LL-ModMenu settings", [] {
                if (const int self = IndexOf("modmenu"); self >= 0) ResetAll(static_cast<size_t>(self));
            }, true).Tip("Puts only LL-ModMenu's own options back to their defaults."),
            Setting::Action("Reset every mod's settings", [] {
                for (size_t i = 0; i < Registry().size(); ++i) ResetAll(i);
            }, true).Tip("Puts all settings of all mods, including these, back to their defaults."),
        }),
    };
}

}
