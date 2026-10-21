#include "modmenu/view/Common.hpp"
#include "modmenu/view/internal/SettingsRows.hpp"

namespace nugget::modmenu::view {

namespace {

struct PageLayout {
    Rect panel{0, 0, 0, 0};
    float w = 0, h = 0, tabW = 0, top = 0, bottom = 0, cx = 0, footY = 0;

    static PageLayout Compute(float w, float h, bool quick, bool tabbed) {
        PageLayout L{Rect{0, w, 0, h}, w, h};
        if (quick) {
            const float margin = Layout::Unit(16.0f, Layout::ScaleFor(w, h));
            L.panel = Layout::Centered(w * 0.5f, h * 0.5f,
                                       std::min(520.0f, std::max(300.0f, w - 2.0f * margin)),
                                       std::max(180.0f, std::min(430.0f, h - 24.0f)));
            L.top = L.panel.y0 + (tabbed ? 64.0f : 40.0f);
            L.bottom = L.panel.y1 - 42.0f;
            L.cx = (L.panel.x0 + L.panel.x1) * 0.5f;
            L.footY = L.panel.y1 - 31.0f;
        } else {
            L.tabW = tabbed ? std::max(100.0f, std::floor(w / 8.0f)) : 0.0f;
            L.top = tabbed ? 29.0f : 33.0f;
            L.bottom = tabbed ? h - 42.0f : h - 33.0f;
            L.cx = L.tabW + (w - L.tabW) * 0.5f;
            L.footY = tabbed ? h - 29.0f : h - 27.0f;
        }
        return L;
    }
};

class SettingsScreen : public Screen {
public:
    SettingsScreen(std::string modId, bool quick)
        : id(std::move(modId)), quick(quick), confirmDialog(std::make_shared<ConfirmationDialog>()) {}

    void OnClose() override { settings::Flush(); }

    bool OnKey(int key) override {
        const int m = ModIdx();
        if (quick && m >= 0 && BaseKey(key) == BaseKey(keybinds::Get(QuickAction(id), Registry()[static_cast<size_t>(m)].quickConfigKey))) {
            MossUI::Close();
            return true;
        }
        if (key == Key(KeyCode::ESCAPE)) { MossUI::Close(); return true; }
        if ((key == K_PGUP || key == K_PGDN) && tabs.size() > 1) {
            SelectTab(std::clamp(cur + (key == K_PGDN ? 1 : -1), 0, static_cast<int>(tabs.size()) - 1));
            return true;
        }
        return list && list->NavKey(key);
    }

    void Init(float w, float h) override {
        const int m = ModIdx();
        if (m < 0) { MossUI::Close(); return; }
        mod = &Registry()[static_cast<size_t>(m)];

        const auto& cachedRows = SettingsOf(*mod);
        auto modRows = cachedRows;
        tabs = settings_rows::BuildTabs(modRows);
        if (tabs.empty()) tabs.push_back({});
        if (settings_rows::HasKeySetting(modRows)) {
            conflictRows = std::make_shared<SettingRows>();
            conflictRows->reserve(Registry().size());
            for (size_t i = 0; i < Registry().size(); ++i) {
                const auto& entry = Registry()[i];
                conflictRows->push_back(i == static_cast<size_t>(m) ? std::move(modRows) : SettingsOf(entry));
            }
        }
        settingsRevision = SettingsRevision(id);
        tabScroll.resize(tabs.size(), 0.0f);
        cur = std::clamp(cur, 0, static_cast<int>(tabs.size()) - 1);
        titles.clear();
        for (const auto& t : tabs) titles.push_back(t.title);

        const bool tabbed = tabs.size() > 1;
        lay = PageLayout::Compute(w, h, quick, tabbed);
        quickTabs = nullptr;

        if (tabbed && quick) {
            auto& tb = root.Add<TabBar>();
            quickTabs = &tb;
            tb.bounds = {lay.panel.x0 + 14.0f, lay.panel.x1 - 14.0f, lay.panel.y0 + 34.0f, lay.panel.y0 + 58.0f};
            tb.tabs = titles;
            tb.selected = cur;
            tb.onSelect = [this](int i) { SelectTab(i); };
        } else if (tabbed) {
            auto& tl = root.Add<TabListView>();
            tl.bounds = {0, lay.tabW, lay.top, lay.bottom};
            tl.titles = &titles;
            tl.current = [this] { return cur; };
            tl.onSelect = [this](int i) { SelectTab(i); };
            if (!mod->website.empty())
                root.Add<ButtonWidget>("Website", [this] { if (mod) ShowLinkPrompt(confirmDialog.get(), mod->website); })
                    .bounds = {4.0f, lay.tabW - 4.0f, lay.footY, lay.footY + 20.0f};
        }

        list = &root.Add<ListWidget>();
        list->bounds = {lay.cx - 156.0f, lay.cx + 156.0f, lay.top + (quick ? 0.0f : 7.0f), lay.bottom};
        FillList();

        const auto footerButtons = Layout::Columns({lay.cx - 155.0f, lay.cx + 155.0f, lay.footY, lay.footY + 20.0f}, 2, 10.0f);
        root.Add<ButtonWidget>("Reset", [this] { ResetAll(static_cast<size_t>(std::max(0, ModIdx()))); })
            .At(footerButtons[0]);
        root.Add<ButtonWidget>("Done", [] { MossUI::Close(); }).At(footerButtons[1]);

        if (!quick) {
            confirmDialog->bounds = {0.0f, w, 0.0f, h};
            root.Add(confirmDialog);
        }
    }

    void Render(UI& ui) override {
        ApplyAccessibilitySettings();
        PreloadSprites(ui);
        if (!mod) { root.Render(ui); return; }
        if (settingsRevision != SettingsRevision(id)) {
            settingsRevision = SettingsRevision(id);
            const float currentScroll = list ? list->ScrollPosition() : 0.0f;
            Rebuild();
            listDirty = true;
            savedRebuildScroll = currentScroll;
        }
        if (cur < static_cast<int>(tabs.size())) {
            std::vector<bool> signature;
            settings_rows::VisibilitySignature(tabs[static_cast<size_t>(cur)].rows, signature);
            if (signature != visibilitySignature) listDirty = true;
        }
        if (listDirty) { listDirty = false; FillList(); }
        if (quick) {
            ui.Fill({0, lay.w, 0, lay.h}, col::BLACK, 0.20f);
            ui.Panel(lay.panel);
            ui.Fill({lay.panel.x0 + 1.0f, lay.panel.x1 - 1.0f, lay.panel.y0 + 29.0f, lay.panel.y0 + 30.0f},
                    CurrentTheme().border, 0.8f);
            ui.Fill({lay.panel.x0 + 1.0f, lay.panel.x1 - 1.0f, lay.panel.y1 - 38.0f, lay.panel.y1 - 37.0f},
                    CurrentTheme().border, 0.8f);
            ui.Text(lay.panel.x0 + 12.0f, lay.panel.x1 - 12.0f, lay.panel.y0 + 10.0f,
                    mod->name + " Settings", CurrentTheme().text, Align::Center, 1.0f);
            if (quickTabs) quickTabs->selected = cur;
            ui.Text(lay.panel.x0 + 12.0f, lay.panel.x1 - 12.0f, lay.panel.y1 - 25.0f,
                    "v" + mod->version, CurrentTheme().muted, Align::Right, 1.0f);
        } else {
            MenuBackground(ui);
            if (!DirtOn(ui.InGame())) ui.Fill({lay.tabW, lay.w, lay.top, lay.bottom}, col::BLACK, 0.30f);
            HeaderSeparator(ui, lay.tabW, lay.w, lay.top);
            FooterSeparator(ui, lay.tabW, lay.w, lay.bottom);
            if (lay.tabW > 0) {
                float y = 4.0f;
                for (const auto& line : ui.Wrap(mod->name + " Settings", lay.tabW - 6.0f)) {
                    ui.Text(0, lay.tabW, y, line, col::WHITE, Align::Center, 1.0f);
                    y += 10.0f;
                }
                ui.Text(lay.tabW, lay.w - 4.0f, lay.top - 9.0f, "v" + mod->version, TEXT_GRAY, Align::Right, 1.0f);
            } else {
                ui.Text(0, lay.w, 12, mod->name, col::WHITE, Align::Center, 1.0f);
            }
        }
        root.Render(ui);
    }

private:
    int ModIdx() const { return IndexOf(id); }
    void SelectTab(int i) {
        if (i == cur || i < 0 || i >= static_cast<int>(tabs.size())) return;
        if (list) tabScroll[static_cast<size_t>(cur)] = list->ScrollPosition();
        cur = i;
        listDirty = true;
    }
    void FillList() {
        if (!list || cur >= static_cast<int>(tabs.size())) return;
        const bool sameTab = lastFilledTab == cur;
        const float oldScroll = sameTab ? list->ScrollPosition() : tabScroll[static_cast<size_t>(cur)];
        list->Clear();
        if (!sameTab) list->ResetScroll();
        else list->SetCursor(-1);
        visibilitySignature.clear();
        settings_rows::VisibilitySignature(tabs[static_cast<size_t>(cur)].rows, visibilitySignature);
        settings_rows::AddRows(*list, tabs[static_cast<size_t>(cur)].rows,
                               static_cast<size_t>(std::max(0, ModIdx())), conflictRows);
        list->SetScrollPosition(sameTab ? oldScroll : std::max(oldScroll, savedRebuildScroll));
        savedRebuildScroll = 0.0f;
        tabScroll[static_cast<size_t>(cur)] = list->ScrollPosition();
        lastFilledTab = cur;
    }

    std::string id;
    bool quick;
    const ModInfo* mod = nullptr;
    std::shared_ptr<ConfirmationDialog> confirmDialog;
    ListWidget* list = nullptr;
    std::vector<settings_rows::Tab> tabs;
    std::shared_ptr<SettingRows> conflictRows;
    std::vector<std::string> titles;
    TabBar* quickTabs = nullptr;
    PageLayout lay{};
    int cur = 0;
    int lastFilledTab = -1;
    bool listDirty = false;
    uint64_t settingsRevision = 0;
    std::vector<float> tabScroll;
    float savedRebuildScroll = 0.0f;
    std::vector<bool> visibilitySignature;
};

} 

std::shared_ptr<MossUI::Screen> MakeSettingsScreen(const std::string& modId, bool quick) {
    return std::make_shared<SettingsScreen>(modId, quick);
}

} 
