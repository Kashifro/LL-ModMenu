#pragma once

#include "modmenu/ModMenu.hpp"
#include "modmenu/internal/Settings.hpp"
#include "modmenu/internal/Launch.hpp"
#include "modmenu/internal/Registry.hpp"

#include "MossUI/MossUI.hpp"
#include "MossUI/core/Layout.hpp"
#include "platform/Log.hpp"

#include "platform/Prefs.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace modmenu::view {

using namespace detail;
using namespace MossUI;

constexpr int K_UP = Key(KeyCode::UP), K_DOWN = Key(KeyCode::DOWN), K_PGUP = Key(KeyCode::PAGE_UP),
              K_PGDN = Key(KeyCode::PAGE_DOWN), K_ENTER = Key(KeyCode::ENTER), K_ESC = Key(KeyCode::ESCAPE);

constexpr const char* DIRT_TEXTURE = "textures/ui/background";
constexpr float MOD_ICON_SIZE = 32.0f;
constexpr float MOD_DETAILS_HEADER_Y = 48.0f;

using col::LINK;
using col::SUMMARY;
using col::TEXT_GRAY;

std::shared_ptr<MossUI::Screen> ModsMenuScreen();
std::shared_ptr<MossUI::Screen> MakeSettingsScreen(const std::string& modId);
void OpenMenu();
void OpenConfig(size_t modIndex);

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
