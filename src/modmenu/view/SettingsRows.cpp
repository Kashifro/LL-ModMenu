#include "modmenu/view/internal/SettingsRows.hpp"

namespace nugget::modmenu::view::settings_rows {

std::vector<Tab> BuildTabs(std::vector<Setting> rows) {
    std::vector<Tab> tabs;
    Tab general{"General", {}};
    for (auto& row : rows) {
        if (row.kind == Setting::Kind::Category) tabs.push_back({row.label, std::move(row.children)});
        else general.rows.push_back(std::move(row));
    }
    if (tabs.empty()) return {{{}, std::move(general.rows)}};
    if (!general.rows.empty()) tabs.insert(tabs.begin(), std::move(general));
    return tabs;
}

void VisibilitySignature(const std::vector<Setting>& rows, std::vector<bool>& signature, bool parentVisible) {
    for (const auto& row : rows) {
        const bool visible = parentVisible && (!row.shownIf || row.shownIf());
        signature.push_back(visible);
        if (row.kind == Setting::Kind::Category) VisibilitySignature(row.children, signature, visible);
    }
}

bool HasKeySetting(const std::vector<Setting>& rows) {
    for (const auto& row : rows)
        if (row.kind == Setting::Kind::Key || HasKeySetting(row.children)) return true;
    return false;
}

void AddRows(ListWidget& list, const std::vector<Setting>& rows, size_t modIndex,
             const std::shared_ptr<SettingRows>& conflictRows) {
    for (const auto& row : rows) {
        if (row.shownIf && !row.shownIf()) continue;

        std::shared_ptr<OptionWidget> widget;
        switch (row.kind) {
            case Setting::Kind::Category:
                list.Add<SeparatorOptionWidget>(row.label);
                AddRows(list, row.children, modIndex, conflictRows);
                continue;
            case Setting::Kind::Header:
                list.Add<SeparatorOptionWidget>(row.label);
                continue;
            case Setting::Kind::Action: {
                auto button = std::make_shared<ConfirmButton>(row.label, row.onAction);
                button->confirm = row.confirm;
                button->glyph = row.glyph;
                if (Opt(OPT_TOOLTIPS)) button->tooltip = row.tooltip;
                button->Accessible(row.label, row.tooltip);
                list.Add(std::move(button));
                continue;
            }
            case Setting::Kind::Toggle:
                widget = std::make_shared<ToggleOptionWidget>(row.label, row.getBool, row.setBool);
                break;
            case Setting::Kind::Slider:
                widget = std::make_shared<SliderOptionWidget>(row.label, row.getFloat, row.setFloat, row.minV,
                                                              row.maxV, row.step, row.decimals, row.suffix);
                break;
            case Setting::Kind::Color:
                widget = std::make_shared<ColorOptionWidget>(row.label, row.getInt, row.setInt);
                break;
            case Setting::Kind::Choice:
                widget = std::make_shared<ChoiceOptionWidget>(row.label, row.options, row.getInt, row.setInt);
                break;
            case Setting::Kind::Text:
                widget = std::make_shared<TextOptionWidget>(row.label, row.getText, row.setText, row.placeholder);
                break;
            case Setting::Kind::Key: {
                auto key = std::make_shared<KeyBindOptionWidget>(row.label, row.getKey, row.setKey);
                key->conflict = [conflictRows, modIndex, self = row.label](int code) {
                    return KeyConflict(code, modIndex, self, *conflictRows);
                };
                widget = std::move(key);
                break;
            }
        }
        if (Opt(OPT_TOOLTIPS)) widget->tooltip = row.tooltip;
        widget->Accessible(row.label, row.tooltip);
        list.Add(std::move(widget));
    }
}

}
