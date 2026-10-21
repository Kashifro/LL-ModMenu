#pragma once

#include "modmenu/view/Common.hpp"

namespace nugget::modmenu::view {

void DrawModHeader(UI& ui, const ModInfo& mod, float rightX, float paneW);
void BuildModDescription(UI& ui, const ModInfo* mod, DescView& desc);

}
