#pragma once

#include "modmenu/model/ModInfo.hpp"

#include <string>

namespace modmenu {

void RegisterMod(ModInfo info);
void DiscoverInstalledMods();
void Open();
void InitModMenuEntry();
bool InGameEnabled();

}
