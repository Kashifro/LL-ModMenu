#pragma once



#include "modmenu/ModMenu.hpp"
#include "modmenu/internal/ModMenuSettings.hpp"
#include "modmenu/internal/Registry.hpp"

#include "MossUI/MossUI.hpp"
#include "MossUI/core/Layout.hpp"
#include "ll/api/mod/NativeMod.h"

#include "nugget/Keybinds.hpp"
#include "nugget/Settings.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace nugget::modmenu::view {

using namespace detail;
using namespace MossUI;

constexpr int K_UP = Key(KeyCode::UP), K_DOWN = Key(KeyCode::DOWN), K_PGUP = Key(KeyCode::PAGE_UP),
              K_PGDN = Key(KeyCode::PAGE_DOWN), K_ENTER = Key(KeyCode::ENTER);

constexpr const char* DIRT_TEXTURE = "textures/ui/background";   
constexpr float MOD_ICON_SIZE = 32.0f;
constexpr float MOD_DETAILS_HEADER_Y = 48.0f;

using col::LINK;
using col::SUMMARY;
using col::TEXT_GRAY;

std::shared_ptr<MossUI::Screen> ModsMenuScreen();
std::shared_ptr<MossUI::Screen> MakeSettingsScreen(const std::string& modId, bool quick);
void OpenMenu();
void OpenConfig(size_t modIndex);
void ToggleQuickScreen(const std::string& modId);
bool QuickScreenIsCurrent();


std::string Lower(std::string s);
std::vector<std::string> SplitAuthors(const std::string& s);
bool HasBadge(const ModInfo& mod, const char* badge);


void PreloadSprites(UI& ui);
void DrawModIcon(UI& ui, const Rect& r, const ModInfo& mod);
void MenuBackground(UI& ui);


bool Opt(const char* key, bool def = true);
void ApplyAccessibilitySettings();
bool DirtOn(bool inGame);

void ShowLinkPrompt(ConfirmationDialog* dialog, const std::string& url);

} 
