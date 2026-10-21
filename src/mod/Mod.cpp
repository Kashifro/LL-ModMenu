
#include "mod/Mod.h"

#include "hooks/Hooks.hpp"
#include "modmenu/ModMenu.hpp"
#include "nugget/Log.hpp"
#include "nugget/Settings.hpp"

#include "MossMC/MossMC.hpp"
#include "MossUI/MossUI.hpp"

#include "ll/api/mod/RegisterHelper.h"

namespace modmenu_mod {

Mod& Mod::getInstance() {
    static Mod instance;
    return instance;
}

bool Mod::load() {
    getSelf().getLogger().debug("Loading LL-ModMenu...");
    nugget::hooks::InstallResourcePackHook(getSelf().getResourceDir());
    return true;
}

bool Mod::enable() {
    auto& logger = getSelf().getLogger();
    logger.info("Enabling LL-ModMenu");
    logger.info("LL-ModMenu resource-pack directory: {}", getSelf().getResourceDir().string());

    MossMC::Install();

    nugget::hooks::InstallEvents();
    nugget::hooks::InstallClientHooks();

    nugget::modmenu::InitModMenuEntry();

    logger.info("LL-ModMenu enabled - look for the Mods button on the start screen");
    return true;
}

bool Mod::disable() {
    auto& logger = getSelf().getLogger();

    nugget::settings::Flush();
    MossUI::Close();   
    nugget::hooks::RemoveEvents();
    nugget::hooks::RemoveClientHooks();

    logger.info("LL-ModMenu disabled");
    return true;
}

} 

LL_REGISTER_MOD(modmenu_mod::Mod, modmenu_mod::Mod::getInstance());
