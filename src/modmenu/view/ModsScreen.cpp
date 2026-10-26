#include "modmenu/view/Common.hpp"
#include "modmenu/view/internal/ModDetails.hpp"

namespace modmenu::view {

namespace {

class ModsScreen : public Screen {
public:
    ModsScreen()
        : search(std::make_shared<SearchField>("Search for mods")),
          confirmDialog(std::make_shared<ConfirmationDialog>()) {}

    bool OnKey(int key) override {
        if (shown.empty()) return false;
        if (key == K_UP || key == K_DOWN || key == K_PGUP || key == K_PGDN) {
            auto it = std::find(shown.begin(), shown.end(), static_cast<size_t>(std::max(0, selected)));
            int cur = it == shown.end() ? 0 : static_cast<int>(it - shown.begin());
            const int step = (key == K_PGUP || key == K_PGDN) ? 5 : 1;
            cur = std::clamp(cur + ((key == K_DOWN || key == K_PGDN) ? step : -step), 0, static_cast<int>(shown.size()) - 1);
            Select(shown[static_cast<size_t>(cur)]);
            list->Reveal(cur);
            return true;
        }
        if (key == K_ENTER && HasSettings(selected)) { OpenConfig(static_cast<size_t>(selected)); return true; }
        return false;
    }

    void Init(float w, float h) override {
        const int W = static_cast<int>(w), H = static_cast<int>(h);
        paneW = static_cast<float>(W / 2 - 8);
        rightX = static_cast<float>(W) - paneW;
        const float listTop = 67.0f, listBottom = static_cast<float>(H) - 36.0f;
        listRect = {0, paneW, listTop, listBottom};
        descRect = {rightX, static_cast<float>(W), MOD_DETAILS_HEADER_Y + 60.0f, listBottom};

        sortDesc = Opt(OPT_SORT_DESC, false);
        configurableOnly = prefs::GetInt(OPT_SHOW, 0) == 1;

        const float sbw = paneW - 32.0f - 22.0f;
        searchX = paneW / 2.0f - sbw / 2.0f - 11.0f;
        search->bounds = {searchX, searchX + sbw, 22, 42};
        root.Add(search);
        root.Add<IconButton>("filter", [this] { filtersShown = !filtersShown; Rebuild(); })
            .At({paneW / 2.0f + sbw / 2.0f - 8.0f, paneW / 2.0f + sbw / 2.0f + 12.0f, 22, 42})
            .Tip("Toggle filter options");

        const float sortW = 80.0f, showW = 112.0f, filtersW = sortW + showW + 2.0f;
        const float searchRowW = searchX + sbw + 22.0f;
        const bool sideBySide = filtersW + 76.0f + 20.0f < searchRowW;
        const float fx = sideBySide ? searchRowW - filtersW + 1.0f : paneW / 2.0f - filtersW / 2.0f;
        countVisible = sideBySide || !filtersShown;
        if (filtersShown) {
            auto& sortBtn = root.Add<ConfirmButton>(sortDesc ? "Z-A" : "A-Z", [this] {
                sortDesc = !sortDesc;
                prefs::SetBool(OPT_SORT_DESC, sortDesc);
                lastQuery = "\x01";
                Rebuild();
            });
            sortBtn.confirm = false;
            sortBtn.glyph = sortDesc ? "sort_za" : "sort_az";
            sortBtn.bounds = {fx, fx + sortW, 45, 65};
            sortBtn.tooltip = std::string("Sort: ") + (sortDesc ? "Z-A" : "A-Z");
            root.Add<ButtonWidget>(std::string("Show: ") + (configurableOnly ? "Configurable" : "All"), [this] {
                configurableOnly = !configurableOnly;
                prefs::SetInt(OPT_SHOW, configurableOnly);
                lastQuery = "\x01";
                Rebuild();
            }).bounds = {fx + sortW + 2.0f, fx + sortW + 2.0f + showW, 45, 65};
        }

        list = &root.Add<RowList>();
        list->bounds = listRect;
        list->count = [this] { return static_cast<int>(shown.size()); };
        list->isSelected = [this](int idx) {
            return idx >= 0 && idx < static_cast<int>(shown.size()) &&
                   static_cast<int>(shown[static_cast<size_t>(idx)]) == selected;
        };
        list->listFocused = [this] { return !search->state.focused; };
        list->drawRow = [this](UI& ui, int idx, const Rect& box) { DrawModRow(ui, shown[static_cast<size_t>(idx)], box); };
        list->onActivate = [this](int idx, float relX, bool dbl) {
            const size_t m = shown[static_cast<size_t>(idx)];
            Select(m);
            if (HasSettings(static_cast<int>(m)) && (relX <= MOD_ICON_SIZE || dbl)) OpenConfig(m);
        };

        desc = &root.Add<DescView>();
        desc->bounds = descRect;
        desc->onOpenLink = [this](const std::string& url) { ShowLinkPrompt(confirmDialog.get(), url); };

        const float btnW = std::floor((paneW - 6.0f) / 2.0f);
        auto slot = [&](int i) {
            const float x = rightX + 2.0f + static_cast<float>(i) * (btnW + 2.0f);
            return Rect{x, x + btnW, MOD_DETAILS_HEADER_Y + 36.0f, MOD_DETAILS_HEADER_Y + 56.0f};
        };
        website = &root.Add<ButtonWidget>("Website", [this] { if (Sel()) ShowLinkPrompt(confirmDialog.get(), Sel()->website); });
        website->bounds = slot(0);
        issues = &root.Add<ButtonWidget>("Issues", [this] { if (Sel()) ShowLinkPrompt(confirmDialog.get(), Sel()->issues); });
        issues->bounds = slot(1);

        configure = &root.Add<IconButton>("configure", [this] { if (HasSettings(selected)) OpenConfig(static_cast<size_t>(selected)); });
        configure->bounds = {static_cast<float>(W) - 24.0f, static_cast<float>(W) - 4.0f,
                             MOD_DETAILS_HEADER_Y, MOD_DETAILS_HEADER_Y + 20.0f};
        configure->tooltip = "Configure...";

        root.Add<ButtonWidget>("Open Mods Folder", [] {
            OpenFolder(Self().getModDir().parent_path());
        }).bounds = {W / 2.0f - 154.0f, W / 2.0f - 4.0f, static_cast<float>(H) - 28.0f, static_cast<float>(H) - 8.0f};
        root.Add<ButtonWidget>("Done", [] { MossUI::Close(); })
            .bounds = {W / 2.0f + 4.0f, W / 2.0f + 154.0f, static_cast<float>(H) - 28.0f, static_cast<float>(H) - 8.0f};

        confirmDialog->bounds = {0.0f, w, 0.0f, h};
        root.Add(confirmDialog);

        lastQuery = "\x01";
        descDirty = true;
    }

    void Render(UI& ui) override {
        ApplyAccessibilitySettings();
        PreloadSprites(ui);
        if (settingsRevision != SettingsRevision()) {
            settingsRevision = SettingsRevision();
            lastQuery = "\x01";
        }
        if (lastQuery != search->state.text) { lastQuery = search->state.text; FillMods(); }
        if (descDirty) {
            descDirty = false;
            BuildModDescription(ui, Sel(), *desc);
        }

        MenuBackground(ui);
        ui.Text(0, paneW, 8, "Mods", col::WHITE, Align::Center, 1.0f);

        const bool has = Sel() != nullptr;
        configure->visible = has && HasSettings(selected);
        website->visible = issues->visible = has;
        if (has) {
            website->enabled = !Sel()->website.empty();
            issues->enabled = !Sel()->issues.empty();
        }

        ListFrame(ui, listRect);
        if (has) ListFrame(ui, descRect);

        if (countVisible && Opt(OPT_COUNT)) {
            const size_t n = shown.size();
            ui.Text(searchX, searchX + 160, 52, "Showing " + std::to_string(n) + (n == 1 ? " mod" : " mods"), col::WHITE, Align::Left, 1.0f);
        }
        if (has) DrawModHeader(ui, *Sel(), rightX, paneW);
        root.Render(ui);
    }

private:
    const ModInfo* Sel() const {
        return selected >= 0 && selected < static_cast<int>(Registry().size()) ? &Registry()[static_cast<size_t>(selected)] : nullptr;
    }
    bool HasSettings(int m) const {
        if (m < 0 || m >= static_cast<int>(Registry().size())) return false;
        const ModInfo& mod = Registry()[static_cast<size_t>(m)];
        return mod.openScreen || mod.settings;
    }
    void Select(size_t m) {
        if (static_cast<int>(m) == selected) return;
        selected = static_cast<int>(m);
        descDirty = true;
        prefs::SetString(OPT_LAST, Registry()[m].id);
    }

    void DrawModRow(UI& ui, size_t m, const Rect& box) {
        const ModInfo& mod = Registry()[m];
        const float ix = list->RowLeft(), iy = box.y0 + (list->itemH - MOD_ICON_SIZE) * 0.5f;
        const bool cfgRow = ui.Hovered(box) && HasSettings(static_cast<int>(m)) &&
                            (!list->NeedsScroll() || ui.MouseX() < list->bounds.x1 - 6.0f);
        const Rect ir{ix, ix + MOD_ICON_SIZE, iy, iy + MOD_ICON_SIZE};
        DrawModIcon(ui, ir, mod);
        if (cfgRow && ui.Hovered(ir)) ui.DrawGlyph(ir, "configure_overlay", true, true, ui.MouseDown());

        const float rowRight = ix + list->RowWidth();
        const float nameX = ix + MOD_ICON_SIZE + 3.0f, maxNameW = list->RowWidth() - MOD_ICON_SIZE - 3.0f;
        const std::string name = ui.Trunc(mod.name, maxNameW);
        ui.Text(nameX, rowRight, iy + 1.0f, name, col::WHITE, Align::Left, 1.0f);
        float bx = nameX + ui.TextWidth(name) + 2.0f;
        if (Opt(OPT_BADGES)) {
            for (const auto& badge : mod.badges) {
                if (bx + ui.TextWidth(badge) + 6.0f >= rowRight) break;
                bx += DrawBadge(ui, bx, iy, badge);
            }
        }

        const int subtitle = ListSubtitleMode();
        const float sx = nameX + 4.0f, sw = list->RowWidth() - MOD_ICON_SIZE - 7.0f;
        auto lines = subtitle == 1 ? std::vector<std::string>{"v" + mod.version}
                                   : subtitle == 0 ? ui.Wrap(mod.description, sw)
                                                   : std::vector<std::string>{};
        for (size_t i = 0; i < lines.size() && i < 2; ++i) {
            const bool cut = i == 1 && lines.size() > 2;
            ui.Text(sx, rowRight, iy + 11.0f + 9.0f * static_cast<float>(i),
                    cut ? ui.Trunc(lines[i] + "...", sw) : lines[i], SUMMARY, Align::Left, 1.0f);
        }
    }

    void FillMods() {
        shown.clear();
        const std::string q = Lower(search->state.text);
        const auto& mods = Registry();
        for (size_t i = 0; i < mods.size(); ++i) {
            if (!Opt(OPT_SHOW_DISABLED) && HasBadge(mods[i], BADGE_DISABLED)) continue;
            if (!Opt(OPT_SHOW_LIBRARIES) && HasBadge(mods[i], BADGE_LIBRARY)) continue;
            if (!Opt(OPT_SHOW_UNLOADED) && HasBadge(mods[i], BADGE_NOT_LOADED)) continue;
            const auto containsQuery = [&q](const std::string& value) {
                return Lower(value).find(q) != std::string::npos;
            };
            bool hit = q.empty() || containsQuery(mods[i].name) || containsQuery(mods[i].id) ||
                       containsQuery(mods[i].version) || containsQuery(mods[i].author) ||
                       (Opt(OPT_SEARCH_DESC) && containsQuery(mods[i].description));
            if (!hit && Opt(OPT_SEARCH_SETTINGS)) {
                std::function<bool(const std::vector<Setting>&)> matchesSetting = [&](const std::vector<Setting>& rows) {
                    for (const auto& row : rows) {
                        if (containsQuery(row.label) || containsQuery(row.tooltip) || matchesSetting(row.children))
                            return true;
                    }
                    return false;
                };
                hit = matchesSetting(SettingsOf(mods[i]));
            }
            if (!hit) continue;
            if (configurableOnly && !HasSettings(static_cast<int>(i))) continue;
            shown.push_back(i);
        }
        if (sortDesc) std::reverse(shown.begin(), shown.end());

        if (selected < 0 && Opt(OPT_REMEMBER, false)) {
            const std::string last = prefs::GetString(OPT_LAST, "");
            for (size_t i : shown) if (mods[i].id == last) { selected = static_cast<int>(i); descDirty = true; break; }
        }

        const auto it = std::find(shown.begin(), shown.end(), static_cast<size_t>(std::max(0, selected)));
        if (selected < 0 || it == shown.end()) {
            const int before = selected;
            selected = shown.empty() ? -1 : static_cast<int>(shown.front());
            if (selected != before) descDirty = true;
            list->ResetScroll();
        } else {
            list->Reveal(static_cast<int>(it - shown.begin()));
        }
    }

    std::shared_ptr<SearchField> search;
    std::shared_ptr<ConfirmationDialog> confirmDialog;
    RowList* list = nullptr;
    DescView* desc = nullptr;
    IconButton* configure = nullptr;
    ButtonWidget* website = nullptr;
    ButtonWidget* issues = nullptr;
    std::vector<size_t> shown;
    Rect listRect{0, 0, 0, 0}, descRect{0, 0, 0, 0};
    float paneW = 0, rightX = 0, searchX = 0;
    int selected = -1;
    bool configurableOnly = false;
    bool sortDesc = false;
    bool filtersShown = false;
    bool countVisible = true;
    bool descDirty = true;
    std::string lastQuery;
    uint64_t settingsRevision = 0;
};

}

std::shared_ptr<MossUI::Screen> ModsMenuScreen() {
    static const std::shared_ptr<ModsScreen> screen = std::make_shared<ModsScreen>();
    return screen;
}

}
