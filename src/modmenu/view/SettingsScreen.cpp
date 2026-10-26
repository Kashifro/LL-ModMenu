#include "modmenu/view/Common.hpp"
#include "modmenu/view/internal/SettingsRows.hpp"

namespace modmenu::view {

namespace {

struct PageLayout {
    Rect panel{0, 0, 0, 0};
    float w = 0, h = 0, tabW = 0, top = 0, bottom = 0, cx = 0, footY = 0;

    static PageLayout Compute(float w, float h, bool tabbed) {
        PageLayout L{Rect{0, w, 0, h}, w, h};
        L.tabW = tabbed ? std::max(100.0f, std::floor(w / 8.0f)) : 0.0f;
        L.top = tabbed ? 29.0f : 33.0f;
        L.bottom = tabbed ? h - 42.0f : h - 33.0f;
        L.cx = L.tabW + (w - L.tabW) * 0.5f;
        L.footY = tabbed ? h - 29.0f : h - 27.0f;
        return L;
    }
};

class SettingsScreen : public Screen {
public:
    explicit SettingsScreen(std::string modId)
        : id(std::move(modId)), confirmDialog(std::make_shared<ConfirmationDialog>()) {}

    bool OnKey(int key) override {
        if (key == K_ESC) { MossUI::Close(); return true; }
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

        tabs = settings_rows::BuildTabs(SettingsOf(*mod));
        if (tabs.empty()) tabs.push_back({});
        settingsRevision = SettingsRevision();
        tabScroll.resize(tabs.size(), 0.0f);
        cur = std::clamp(cur, 0, static_cast<int>(tabs.size()) - 1);
        titles.clear();
        for (const auto& t : tabs) titles.push_back(t.title);

        const bool tabbed = tabs.size() > 1;
        lay = PageLayout::Compute(w, h, tabbed);

        if (tabbed) {
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
        list->bounds = {lay.cx - 156.0f, lay.cx + 156.0f, lay.top + 7.0f, lay.bottom};
        FillList();

        const auto footerButtons = Layout::Columns({lay.cx - 155.0f, lay.cx + 155.0f, lay.footY, lay.footY + 20.0f}, 2, 10.0f);
        root.Add<ButtonWidget>("Reset", [this] { ResetAll(static_cast<size_t>(std::max(0, ModIdx()))); })
            .At(footerButtons[0]);
        root.Add<ButtonWidget>("Done", [] { MossUI::Close(); }).At(footerButtons[1]);

        confirmDialog->bounds = {0.0f, w, 0.0f, h};
        root.Add(confirmDialog);
    }

    void Render(UI& ui) override {
        ApplyAccessibilitySettings();
        PreloadSprites(ui);
        if (!mod) { root.Render(ui); return; }
        if (settingsRevision != SettingsRevision()) {
            settingsRevision = SettingsRevision();
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
        settings_rows::AddRows(*list, tabs[static_cast<size_t>(cur)].rows);
        list->SetScrollPosition(sameTab ? oldScroll : std::max(oldScroll, savedRebuildScroll));
        savedRebuildScroll = 0.0f;
        tabScroll[static_cast<size_t>(cur)] = list->ScrollPosition();
        lastFilledTab = cur;
    }

    std::string id;
    const ModInfo* mod = nullptr;
    std::shared_ptr<ConfirmationDialog> confirmDialog;
    ListWidget* list = nullptr;
    std::vector<settings_rows::Tab> tabs;
    std::vector<std::string> titles;
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

std::shared_ptr<MossUI::Screen> MakeSettingsScreen(const std::string& modId) {
    return std::make_shared<SettingsScreen>(modId);
}

}
