#pragma once

#include "modmenu/view/Common.hpp"

namespace nugget::modmenu::view::settings_rows {

struct Tab {
    std::string title;
    std::vector<Setting> rows;
};

std::vector<Tab> BuildTabs(std::vector<Setting> rows);
void VisibilitySignature(const std::vector<Setting>& rows, std::vector<bool>& signature, bool parentVisible = true);
bool HasKeySetting(const std::vector<Setting>& rows);
void AddRows(ListWidget& list, const std::vector<Setting>& rows, size_t modIndex,
             const std::shared_ptr<SettingRows>& conflictRows);

}
