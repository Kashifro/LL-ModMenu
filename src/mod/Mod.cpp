#include "mod/Mod.hpp"

#include "hooks/Hooks.hpp"
#include "modmenu/ModMenu.hpp"
#include "platform/Log.hpp"

#include "MossMC/MossMC.hpp"
#include "MossUI/MossUI.hpp"

#include "ll/api/mod/RegisterHelper.h"

namespace modmenu_mod {

Mod& Mod::getInstance() {
    static Mod instance;
    return instance;
}

bool Mod::load() {
    modmenu::Log().debug("Loading LL-ModMenu...");
    modmenu::hooks::InstallResourcePackHook(getSelf().getResourceDir());
    return true;
}

bool Mod::enable() {
    auto& logger = modmenu::Log();
    logger.info("Enabling LL-ModMenu");
    logger.info("LL-ModMenu resource-pack directory: {}", getSelf().getResourceDir().string());

    MossMC::Install();

    modmenu::hooks::InstallEvents();
    modmenu::hooks::InstallClientHooks();

    modmenu::InitModMenuEntry();

    logger.info("LL-ModMenu enabled  look for the Mods button on the start screen");
    return true;
}

bool Mod::disable() {
    auto& logger = modmenu::Log();

    MossUI::Close();
    modmenu::hooks::RemoveEvents();
    modmenu::hooks::RemoveClientHooks();

    logger.info("LL-ModMenu disabled");
    return true;
}

}

LL_REGISTER_MOD(modmenu_mod::Mod, modmenu_mod::Mod::getInstance());
