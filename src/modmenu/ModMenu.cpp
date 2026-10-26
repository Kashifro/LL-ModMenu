#include "modmenu/ModMenu.hpp"
#include "modmenu/internal/Settings.hpp"
#include "modmenu/internal/Registry.hpp"
#include "modmenu/view/Common.hpp"

#include "MossUI/MossUI.hpp"

#include "platform/Keybinds.hpp"
#include "platform/Log.hpp"
#include "platform/Prefs.hpp"

namespace modmenu {

using namespace detail;
using MossUI::Key;
using MossUI::KeyCode;

bool InGameEnabled() { return prefs::GetBool(OPT_INGAME, false); }

void Open() {
    DiscoverInstalledMods();
    view::OpenMenu();
}

void InitModMenuEntry() {
    MossUI::SetAssetDir(Self().getModDir() / "assets");

    ModInfo self;
    self.id = "modmenu";
    self.name = "LL-ModMenu";
    self.description = "Browse installed mods and configure their settings.";
    self.version = SelfVersion();
    self.author = "Kashifro";
    self.icon = ResolveModIcon(Self().getModDir(), "icon.png");
    self.settings = BuildModMenuSettings;
    RegisterMod(std::move(self));
    DiscoverInstalledMods();

    keybinds::Register(OPEN_KEY_NAME, Key(KeyCode::M));
}

}
