#pragma once

#include <filesystem>
#include <string>

namespace modmenu::detail {

void OpenUri(const std::string& uri);
void OpenFolder(const std::filesystem::path& folder);

}
