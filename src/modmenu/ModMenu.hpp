#pragma once

#include "modmenu/model/ModInfo.hpp"

#include <string>

namespace nugget::modmenu {

void RegisterMod(ModInfo info);
void RefreshModSettings(const std::string& modId);
void DiscoverInstalledMods();
bool IsOpen();
void Open();
void Close();
void Toggle();
void InitModMenuEntry();
bool InGameEnabled();

}
