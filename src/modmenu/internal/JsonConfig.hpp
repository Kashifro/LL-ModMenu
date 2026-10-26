#pragma once

#include "modmenu/model/Setting.hpp"

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace modmenu::detail {

using Notify = std::function<void(const std::string& path)>;
std::vector<Setting> JsonConfigSettings(const std::filesystem::path& file, Notify notify = {});

}