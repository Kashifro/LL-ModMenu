#include "modmenu/internal/Settings.hpp"

#include "modmenu/internal/Launch.hpp"
#include "modmenu/internal/Registry.hpp"

#include "platform/Log.hpp"
#include "platform/Prefs.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace modmenu::detail {

int ListSubtitleMode() {
    return std::clamp(prefs::GetInt(OPT_LIST_SUBTITLE, 0), 0, 2);
}

namespace {
auto Flag(std::string label, const char* key, bool def, std::string tip) {
    return Setting::Toggle(std::move(label), [key, def] { return prefs::GetBool(key, def); },
                           [key](bool value) { prefs::SetBool(key, value); }, def)
        .Tip(std::move(tip));
}

auto Percent(std::string label, const char* key, int def, std::string tip, float lo = 0, float hi = 100, float step = 1) {
    return Setting::Slider(std::move(label), [key, def] { return static_cast<float>(prefs::GetInt(key, def)); },
                           [key](float value) { prefs::SetInt(key, static_cast<int>(std::lround(value))); },
                           lo, hi, static_cast<float>(def), step, 0, "%")
        .Tip(std::move(tip));
}

}

std::vector<Setting> BuildModMenuSettings() {
    return {
        Setting::Category("Appearance", {
            Flag("Dirt background", OPT_DIRTBG, true, "Tiled dirt behind the mod list and settings pages. Off = plain dim."),
            Percent("Background darkness", OPT_DIM, 55, "How dark the backdrop is on the start screen."),
            Percent("Text size", OPT_TEXT_SCALE, 100, "Scale MossUI text from 75% to 200%.", 75, 200, 5),
            Flag("Badges", OPT_BADGES, true, "Client, Library, Disabled... tags next to mod names."),
            Setting::Choice("Mod list subtitle", {"Summary", "Version", "None"},
                            [] { return ListSubtitleMode(); },
                            [](int value) { prefs::SetInt(OPT_LIST_SUBTITLE, value); }, 0)
                .Tip("Choose what appears below each mod name."),
            Flag("Mod count", OPT_COUNT, true, "\"Showing N mods\" under the search box."),
            Flag("Tooltips", OPT_TOOLTIPS, true, "Hover text on setting rows."),
        }),
        Setting::Category("Mod details", {
            Flag("Show links", OPT_SHOW_LINKS, true, "Show source and other metadata links."),
            Flag("Show license", OPT_SHOW_LICENSE, true, "Show the mod's license in its details."),
            Flag("Show credits", OPT_SHOW_CREDITS, true, "Show authors in the details pane and header."),
        }),
        Setting::Category("Mod list", {
            Flag("Show disabled mods", OPT_SHOW_DISABLED, true, "Mods LeviLamina has switched off."),
            Flag("Show libraries", OPT_SHOW_LIBRARIES, true, "Preload libraries other mods depend on."),
            Flag("Show mods that aren't loaded", OPT_SHOW_UNLOADED, true, "Folders in mods/ that LeviLamina didn't load."),
            Flag("Search descriptions", OPT_SEARCH_DESC, true, "Search matches description text, not just names."),
            Flag("Search setting names", OPT_SEARCH_SETTINGS, true, "Search also matches the names of a mod's settings."),
            Flag("Remember last mod", OPT_REMEMBER, false, "Reopen the list on the mod you looked at last."),
        }),
        Setting::Category("Sound", {
            Flag("Click sounds", OPT_CLICK, true, "Vanilla click on buttons, switches and list entries."),
            Percent("Click volume", OPT_CLICK_VOLUME, 100, "Volume of the click."),
        }),
        Setting::Category("In game", {
            Flag("Enable in game", OPT_INGAME, false,
                 "Lets LL-ModMenu open over the game with the key set in Settings > Controls (default M). Off hides the options below and disables the key."),
            Flag("Dirt background in game", OPT_DIRTBG_INGAME, false, "Same for screens opened over the game. Off keeps the world visible.")
                .ShownIf([] { return InGameEnabled(); }),
            Percent("Background darkness in game", OPT_DIM_INGAME, 30, "How dark the backdrop is over the game.")
                .ShownIf([] { return InGameEnabled(); }),
        }),
        Setting::Category("Data", {
            Setting::Section("Folders"),
            Setting::Action("Open data folder", [] { OpenFolder(Self().getDataDir()); })
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
