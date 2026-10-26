#include "modmenu/view/internal/SettingsRows.hpp"

#include "platform/Keybinds.hpp"

#include <algorithm>
#include <iterator>

namespace modmenu::view::settings_rows {

std::vector<Tab> BuildTabs(std::vector<Setting> rows) {
    std::vector<Tab> tabs;
    std::vector<Setting> loose; // top-level rows that are not inside a category
    for (auto& row : rows) {
        if (row.kind == Setting::Kind::Category) tabs.push_back({row.label, std::move(row.children)});
        else loose.push_back(std::move(row));
    }
    if (tabs.empty()) return {{{}, std::move(loose)}};
    if (loose.empty()) return tabs;

    // Loose rows join a category already called "General"; otherwise they get a General tab of their own.
    const auto general = std::find_if(tabs.begin(), tabs.end(), [](const Tab& t) { return t.title == "General"; });
    if (general == tabs.end()) tabs.insert(tabs.begin(), Tab{"General", std::move(loose)});
    else general->rows.insert(general->rows.end(), std::make_move_iterator(loose.begin()),
                              std::make_move_iterator(loose.end()));
    return tabs;
}

void VisibilitySignature(const std::vector<Setting>& rows, std::vector<bool>& signature, bool parentVisible) {
    for (const auto& row : rows) {
        const bool visible = parentVisible && (!row.shownIf || row.shownIf());
        signature.push_back(visible);
        if (row.kind == Setting::Kind::Category) VisibilitySignature(row.children, signature, visible);
    }
}

void AddRows(ListWidget& list, const std::vector<Setting>& rows) {
    for (const auto& row : rows) {
        if (row.shownIf && !row.shownIf()) continue;

        std::shared_ptr<OptionWidget> widget;
        switch (row.kind) {
            case Setting::Kind::Category:
                list.Add<SeparatorOptionWidget>(row.label);
                AddRows(list, row.children);
                continue;
            case Setting::Kind::Section:
                list.Add<SeparatorOptionWidget>(row.label);
                continue;
            case Setting::Kind::Action: {
                auto button = std::make_shared<ConfirmButton>(row.label, row.onAction);
                button->confirm = row.confirm;
                if (Opt(OPT_TOOLTIPS)) button->tooltip = row.tooltip;
                button->Accessible(row.label, row.tooltip);
                list.Add(std::move(button));
                continue;
            }
            case Setting::Kind::Toggle:
                widget = std::make_shared<ToggleOptionWidget>(row.label, row.getBool, row.setBool);
                break;
            case Setting::Kind::Slider:
                widget = std::make_shared<SliderOptionWidget>(row.label, row.getFloat, row.setFloat, row.min,
                                                              row.max, row.step, row.decimals, row.unit);
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
                key->conflict = keybinds::Conflict;
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
