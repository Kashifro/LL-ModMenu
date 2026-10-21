#include "modmenu/api/ApiInternal.hpp"

#include "modmenu/ModMenu.hpp"
#include "modmenu/internal/ModMenuSettings.hpp"
#include "modmenu/internal/Registry.hpp"

#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace nugget::modmenu::api {
namespace {

using nugget::modmenu::ModInfo;
using nugget::modmenu::Setting;

template <class Get, class Set>
bool ValidCallbackPair(Get get, Set set) { return get && set; }

bool ValidateSetting(const ModMenuSettingV1& row, size_t depth, size_t& budget) {
    if (budget == 0) return false;
    --budget;
    if (depth > 16 || row.struct_size < sizeof(ModMenuSettingV1) || !row.label) return false;
    if (row.option_count > 128 || row.child_count > 512) return false;
    if ((row.option_count && !row.options) || (row.child_count && !row.children)) return false;

    switch (row.kind) {
        case MODMENU_SETTING_TOGGLE: return ValidCallbackPair(row.get_bool, row.set_bool);
        case MODMENU_SETTING_KEY:
        case MODMENU_SETTING_COLOR: return ValidCallbackPair(row.get_int, row.set_int);
        case MODMENU_SETTING_CHOICE:
            return ValidCallbackPair(row.get_int, row.set_int) && row.option_count > 0;
        case MODMENU_SETTING_SLIDER:
            return ValidCallbackPair(row.get_float, row.set_float) && std::isfinite(row.min_value) &&
                   std::isfinite(row.max_value) && std::isfinite(row.default_float) && std::isfinite(row.step) &&
                   row.min_value <= row.max_value && row.default_float >= row.min_value &&
                   row.default_float <= row.max_value && row.step >= 0 && row.decimals >= 0;
        case MODMENU_SETTING_TEXT: return ValidCallbackPair(row.get_text, row.set_text);
        case MODMENU_SETTING_ACTION: return row.action != nullptr;
        case MODMENU_SETTING_SECTION: return true;
        case MODMENU_SETTING_CATEGORY:
            for (size_t i = 0; i < row.child_count; ++i)
                if (!ValidateSetting(row.children[i], depth + 1, budget)) return false;
            return true;
        default: return false;
    }
}

Setting ConvertSetting(const ModMenuSettingV1& row, const ActiveToken& active, size_t depth) {
    const std::string label = CopyText(row.label);
    const auto userData = row.user_data;
    Setting converted;

    switch (row.kind) {
        case MODMENU_SETTING_TOGGLE: {
            const auto get = row.get_bool;
            const auto set = row.set_bool;
            const bool def = row.default_bool != 0;
            converted = Setting::Toggle(label,
                [active, userData, get, def] { return active->load() ? get(userData) != 0 : def; },
                [active, userData, set](bool value) { if (active->load()) set(userData, value ? 1 : 0); }, def);
            break;
        }
        case MODMENU_SETTING_KEY: {
            const auto get = row.get_int;
            const auto set = row.set_int;
            const int def = row.default_int;
            converted = Setting::KeyBind(label,
                [active, userData, get, def] { return active->load() ? get(userData) : def; },
                [active, userData, set](int value) { if (active->load()) set(userData, value); }, def);
            break;
        }
        case MODMENU_SETTING_SLIDER: {
            const auto get = row.get_float;
            const auto set = row.set_float;
            const float def = row.default_float;
            converted = Setting::Slider(label,
                [active, userData, get, def] { return active->load() ? get(userData) : def; },
                [active, userData, set](float value) { if (active->load()) set(userData, value); },
                row.min_value, row.max_value, def, row.step, row.decimals, CopyText(row.suffix));
            break;
        }
        case MODMENU_SETTING_SECTION:
            converted = Setting::Section(label);
            break;
        case MODMENU_SETTING_COLOR:
        case MODMENU_SETTING_CHOICE: {
            const auto get = row.get_int;
            const auto set = row.set_int;
            const int def = row.default_int;
            if (row.kind == MODMENU_SETTING_COLOR) {
                converted = Setting::Color(label,
                    [active, userData, get, def] { return active->load() ? get(userData) : def; },
                    [active, userData, set](int value) { if (active->load()) set(userData, value); }, def);
            } else {
                std::vector<std::string> options;
                options.reserve(row.option_count);
                for (size_t i = 0; i < row.option_count; ++i) options.push_back(CopyText(row.options[i]));
                converted = Setting::Choice(label, std::move(options),
                    [active, userData, get, def] { return active->load() ? get(userData) : def; },
                    [active, userData, set](int value) { if (active->load()) set(userData, value); }, def);
            }
            break;
        }
        case MODMENU_SETTING_CATEGORY: {
            std::vector<Setting> children;
            children.reserve(row.child_count);
            for (size_t i = 0; i < row.child_count; ++i)
                children.push_back(ConvertSetting(row.children[i], active, depth + 1));
            converted = Setting::Category(label, std::move(children));
            break;
        }
        case MODMENU_SETTING_ACTION: {
            const auto action = row.action;
            converted = Setting::Action(label, [active, userData, action] {
                if (active->load()) action(userData);
            }, row.confirm_action != 0);
            if (row.glyph) converted.glyph = row.glyph;
            break;
        }
        case MODMENU_SETTING_TEXT: {
            const auto get = row.get_text;
            const auto set = row.set_text;
            const std::string def = CopyText(row.default_text);
            converted = Setting::Text(label,
                [active, userData, get, def] { return active->load() ? CopyText(get(userData)) : def; },
                [active, userData, set](const std::string& value) {
                    if (active->load()) set(userData, value.c_str());
                }, def, CopyText(row.placeholder));
            break;
        }
    }

    if (row.tooltip) converted.tooltip = row.tooltip;
    if (row.shown_if) {
        const auto shown = row.shown_if;
        converted.shownIf = [active, userData, shown] { return active->load() && shown(userData) != 0; };
    }
    return converted;
}

}

bool RegisterProvider(const ModMenuModInfoV1* source, ModMenuCustomScreenBuildV2 build,
                      void* customScreenUserData) {
    if (!source || source->struct_size < sizeof(ModMenuModInfoV1) || !source->id || !*source->id ||
        !source->name || !*source->name || source->setting_count > 512 ||
        (source->setting_count && !source->settings)) return false;

    size_t rowBudget = 4096;
    for (size_t i = 0; i < source->setting_count; ++i)
        if (!ValidateSetting(source->settings[i], 0, rowBudget)) return false;

    try {
        const std::string id = source->id;
        auto active = std::make_shared<std::atomic_bool>(true);
        std::vector<Setting> rows;
        rows.reserve(source->setting_count + (build ? 1 : 0));
        for (size_t i = 0; i < source->setting_count; ++i)
            rows.push_back(ConvertSetting(source->settings[i], active, 0));
        if (build) {
            rows.push_back(Setting::Action("Open custom screen", [active, id] {
                if (active->load()) OpenCustomScreen(id.c_str());
            }));
        }

        ModInfo info;
        info.id = id;
        info.name = source->name;
        info.description = CopyText(source->description);
        if (source->version && *source->version) info.version = source->version;
        info.author = CopyText(source->author);
        info.icon = CopyText(source->icon);
        info.website = CopyText(source->website);
        info.issues = CopyText(source->issues);
        info.source = CopyText(source->source);
        info.license = CopyText(source->license);
        info.quickConfigKey = source->quick_config_key;
        info.settings = [rows = std::move(rows)] { return rows; };

        Registration previous = FindRegistration(id);
        StoreRegistration(id, {active, build, customScreenUserData, info.name});
        try {
            nugget::modmenu::RegisterMod(std::move(info));
        } catch (...) {
            active->store(false);
            if (previous.active) StoreRegistration(id, std::move(previous));
            else EraseRegistration(id);
            return false;
        }
        if (previous.active) previous.active->store(false);
        return true;
    } catch (...) {
        return false;
    }
}

void UnregisterProvider(const char* idValue) {
    if (!idValue || !*idValue) return;
    const std::string id = idValue;
    auto registration = FindRegistration(id);
    if (registration.active) registration.active->store(false);
    EraseRegistration(id);

    for (auto& mod : detail::Registry()) {
        if (mod.id != id) continue;
        mod.settings = {};
        detail::InvalidateSettings(id);
        return;
    }
}

}
