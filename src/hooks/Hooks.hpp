#pragma once

#include <filesystem>

namespace modmenu::hooks {

void InstallEvents();
void RemoveEvents();

void InstallResourcePackHook(const std::filesystem::path& resourceDir);
void InstallClientHooks();
void RemoveClientHooks();

}
