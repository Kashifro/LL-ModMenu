#include "modmenu/view/Common.hpp"

namespace nugget::modmenu::view {

namespace {
MossUI::Screen* quickScreen = nullptr;

std::string DirtTexture() {
    const std::string asset = AssetPath("dirtbg");
    return asset.empty() ? DIRT_TEXTURE : asset;
}
}

void OpenMenu() {
    quickScreen = nullptr;
    MossUI::Open(ModsMenuScreen());
}

void OpenConfig(size_t modIndex) {
    const auto& mods = Registry();
    if (modIndex < mods.size()) MossUI::PushScreen(MakeSettingsScreen(mods[modIndex].id, false));
}

void ToggleQuickScreen(const std::string& modId) {
    if (QuickScreenIsCurrent()) { MossUI::Close(); return; }
    if (MossUI::IsOpen()) return;
    auto screen = MakeSettingsScreen(modId, true);
    quickScreen = screen.get();
    MossUI::Open(screen);
}

bool QuickScreenIsCurrent() { return quickScreen && MossUI::Current() == quickScreen; }

std::string Lower(std::string s) {
    for (auto& ch : s) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return s;
}

void PreloadSprites(UI& ui) {
    SetAssetDir(ll::mod::NativeMod::current()->getModDir() / "assets");
    ui.Preload(DirtTexture());
    static constexpr const char* names[] = {
        "configure", "configure_overlay", "filter", "sort_az", "sort_za"
    };
    static constexpr const char* states[] = {"", "_hover", "_pressed"};
    for (const char* name : names) {
        for (const char* state : states) {
            const std::string path = AssetPath(std::string(name) + state);
            if (!path.empty()) ui.Preload(path);
        }
    }
}

void DrawModIcon(UI& ui, const Rect& r, const ModInfo& mod) {
    const bool externalFile = std::filesystem::path(mod.icon).is_absolute();
    if (!mod.icon.empty() && (mod.icon.rfind("textures/", 0) == 0 || externalFile) && ui.Image(r, mod.icon)) return;
    ui.Fill(r, col::ACCENT[std::hash<std::string>{}(mod.id) % 4]);
    const auto first = std::find_if(mod.name.begin(), mod.name.end(), [](unsigned char c) { return std::isalpha(c); });
    const std::string initial = first == mod.name.end() ? "?" : std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(*first))));
    ui.Text(r.x0, r.x1, r.y0 + ((r.y1 - r.y0) - 8.0f) * 0.5f, initial, col::WHITE, Align::Center, 1.0f);
}

std::vector<std::string> SplitAuthors(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    auto flush = [&] {
        size_t a = cur.find_first_not_of(' '), b = cur.find_last_not_of(' ');
        if (a != std::string::npos) out.push_back(cur.substr(a, b - a + 1));
        cur.clear();
    };
    for (char ch : s) { if (ch == ',') flush(); else cur += ch; }
    flush();
    return out;
}

bool Opt(const char* key, bool def) { return settings::GetBool(key, def); }
void ApplyAccessibilitySettings() {
    SetAccessibilityOptions({static_cast<float>(settings::GetInt(OPT_TEXT_SCALE, 100)) / 100.0f});
}
bool HasBadge(const ModInfo& mod, const char* badge) { return std::find(mod.badges.begin(), mod.badges.end(), badge) != mod.badges.end(); }

void ShowLinkPrompt(ConfirmationDialog* dialog, const std::string& url) {
    if (!dialog || (url.rfind("https://", 0) != 0 && url.rfind("http://", 0) != 0)) return;
    FocusWidget(nullptr);
    dialog->Show(
        "Are you sure you want to open the following website?", url,
        {{"Yes", [url] { OpenUri(url); }},
         {"Copy to Clipboard", [url] { SetClipboardText(url); }},
         {"No", {}}},
        "Never open links from people that you don't trust!", DialogPresentation::Overlay);
}

bool DirtOn(bool inGame) { return settings::GetBool(inGame ? OPT_DIRT_INGAME : OPT_DIRT, !inGame); }

void MenuBackground(UI& ui) {
    const bool inGame = ui.InGame();
    const float dim = static_cast<float>(settings::GetInt(inGame ? OPT_DIM_INGAME : OPT_DIM, inGame ? 30 : 55)) / 100.0f;
    DrawMenuBackground(ui, {0, ui.Width(), 0, ui.Height()}, DirtOn(inGame) ? DirtTexture() : "", dim);
}

} 
