#include "modmenu/ModMenu.hpp"
#include "modmenu/internal/ModMenuSettings.hpp"
#include "modmenu/internal/Registry.hpp"
#include "modmenu/view/Common.hpp"

#include "MossUI/MossUI.hpp"
#include "ll/api/mod/NativeMod.h"

#include "nugget/ClientContext.hpp"
#include "nugget/Keybinds.hpp"
#include "nugget/Settings.hpp"

namespace nugget::modmenu {

using namespace detail;
using MossUI::Key;
using MossUI::KeyCode;
namespace ctx = nugget::clientctx;

bool InGameEnabled() { return settings::GetBool(OPT_INGAME, false); }

void RefreshModSettings(const std::string& modId) { detail::InvalidateSettings(modId); }

bool IsOpen() { return MossUI::IsOpen() && !view::QuickScreenIsCurrent(); }

void Open() {
    DiscoverInstalledMods();   
    view::OpenMenu();
}

void Close() { if (IsOpen()) MossUI::Close(); }
void Toggle() { IsOpen() ? Close() : Open(); }

void InitModMenuEntry() {
    settings::SetImmediateWrites(settings::GetBool(OPT_SAVE_IMMEDIATE, true));
    MossUI::SetAssetDir(ll::mod::NativeMod::current()->getModDir() / "assets");

    
    ModInfo self;
    self.id = "modmenu";
    self.name = "LL-ModMenu";
    self.description = "Browse installed mods and configure their settings.";
    self.version = SelfVersion();
    self.author = "Kashifro";
    self.icon = ResolveModIcon(ll::mod::NativeMod::current()->getModDir(), "icon.png");
    self.settings = BuildModMenuSettings;
    RegisterMod(std::move(self));
    DiscoverInstalledMods();

    keybinds::Register(OPEN_KEY_NAME, Key(KeyCode::M), [] {
        if (InGameEnabled() && ctx::IsOnScreen("hud_screen") && !MossUI::IsOpen()) Open();
    });

    
    for (const auto& mod : Registry()) {
        if (!mod.quickConfigKey) continue;
        const std::string id = mod.id;
        keybinds::Register(QuickAction(id), mod.quickConfigKey, [id] {
            if (InGameEnabled() && ctx::IsOnScreen("hud_screen")) view::ToggleQuickScreen(id);
        });
    }
}

} 
