#pragma once

#include "modmenu/view/Common.hpp"

namespace modmenu::view::settings_rows {

struct Tab {
    std::string title;
    std::vector<Setting> rows;
};

std::vector<Tab> BuildTabs(std::vector<Setting> rows);
void VisibilitySignature(const std::vector<Setting>& rows, std::vector<bool>& signature, bool parentVisible = true);
void AddRows(ListWidget& list, const std::vector<Setting>& rows);

}
