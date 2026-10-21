#pragma once

#include "modmenu/ModMenu.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace nugget::modmenu::detail {

std::string NormName(const std::string& name);
std::vector<ModInfo>& Registry();
int IndexOf(const std::string& id);   
std::string SelfVersion();            
std::string ResolveModIcon(const std::filesystem::path& modDir, const std::string& icon);

void OpenUri(const std::string& uri);
void OpenFolder(const std::filesystem::path& folder);

}
