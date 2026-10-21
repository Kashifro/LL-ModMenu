#pragma once

#include "modmenu/model/Setting.hpp"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace nugget::modmenu {

struct ModInfo {
    std::string id;
    std::string name;
    std::string description;
    std::string version = "1.0.0";
    std::string author;
    std::string icon;
    std::function<std::vector<Setting>()> settings;
    int quickConfigKey = 0;
    std::string website;
    std::string issues;
    std::string source;
    std::string license;
    std::vector<std::pair<std::string, std::string>> links;
    std::vector<std::string> badges = {"Client"};
};

}
