# LL-ModMenu registry and settings API

LL-ModMenu exposes a versioned C ABI so another mod can register settings without linking against `ModMenu.dll` or sharing C++ library types across DLLs. The provider owns its values and persistence. LL-ModMenu builds the controls and calls the provider's getter and setter functions.

Include `src/modmenu/api/ModMenuAPI.h` from this repository or the LL-ModMenu source package. Resolve `ModMenu_GetApi` when the provider enables, then register its settings. Register and unregister on the mod lifecycle thread. The older `modmenu/ModMenuAPI.h` include remains available as a compatibility wrapper.

Declare LL-ModMenu as an optional dependency so LeviLamina loads it first when present and protects the integration during lifecycle operations:

```json
"optionalDependencies": [
  { "name": "LL-ModMenu", "version": ">=1.0.0" }
]
```

```cpp
#include "modmenu/api/ModMenuAPI.h"

#include <Windows.h>

namespace {

bool enabled = true;

uint8_t GetEnabled(void*) { return enabled; }
void SetEnabled(void*, uint8_t value) { enabled = value != 0; }

ModMenuApiV1 const* api = nullptr;

}

bool RegisterSettings() {
    const auto module = GetModuleHandleW(L"ModMenu.dll");
    if (!module) return false;

    const auto getApi = reinterpret_cast<const void* (*)(uint32_t)>(
        GetProcAddress(module, "ModMenu_GetApi"));
    if (!getApi) return false;

    api = static_cast<const ModMenuApiV1*>(getApi(MODMENU_API_VERSION_1));
    if (!api || api->struct_size < sizeof(ModMenuApiV1)) return false;

    ModMenuSettingV1 setting{};
    setting.struct_size = sizeof(setting);
    setting.kind = MODMENU_SETTING_TOGGLE;
    setting.label = "Enabled";
    setting.user_data = nullptr;
    setting.get_bool = GetEnabled;
    setting.set_bool = SetEnabled;
    setting.default_bool = 1;

    ModMenuModInfoV1 info{};
    info.struct_size = sizeof(info);
    info.id = "examplemod";
    info.name = "Example Mod";
    info.settings = &setting;
    info.setting_count = 1;

    return api->register_mod(&info) != 0;
}

void UnregisterSettings() {
    if (api) api->unregister_mod("examplemod");
    api = nullptr;
}
```

Call `RegisterSettings()` from the provider's enable callback and `UnregisterSettings()` from disable and before unload. The API copies the metadata and row descriptions during registration; callback functions and their `user_data` must stay valid until unregister. The API disables callbacks on unregister, including callbacks retained by an already-open settings screen.

Supported rows are toggles, keybinds, sliders, colors, choices, text, categories, actions, and section labels. Use `shown_if` for conditional rows. If the settings structure changes, register the mod again with the updated rows.

Settings are not saved by LL-ModMenu. Getter and setter callbacks should access the provider's live configuration, and the provider should persist changes using its own config system.

## Custom screen

Use API V2 when a mod needs a different layout or additional flow. The custom screen is hosted by LL-ModMenu and built with a C ABI builder, so the provider does not pass a `MossUI::Screen` object across DLLs. The mod's normal settings page gets an **Open custom screen** row automatically. A provider can also call `open_custom_screen(id)` directly.

The builder supports fixed labels, live labels, buttons, toggles, sliders, choices, colors, and text fields. Use `add_dynamic_label` with a `ModMenuGetTextV1` callback for text that changes while the page is open. The custom page uses GUI-coordinate rectangles; reserve the top 36 px for its title and bottom 36 px for LL-ModMenu's Done button. Escape also closes it. Builder methods are valid only during the build callback.

```cpp
void BuildCustomScreen(void*, const ModMenuScreenBuilderV2* builder, float width, float height) {
    builder->add_label(builder->context, {20, width - 20, 42, 64}, "Example controls", 1);
    builder->add_toggle(builder->context, {30, width - 30, 72, 108}, "Enabled",
                        GetEnabled, SetEnabled, nullptr);
}

bool RegisterCustomScreen(const ModMenuModInfoV1& mod) {
    const auto module = GetModuleHandleW(L"ModMenu.dll");
    if (!module) return false;
    const auto getApi = reinterpret_cast<const void* (*)(uint32_t)>(
        GetProcAddress(module, "ModMenu_GetApi"));
    if (!getApi) return false;

    const auto api = static_cast<const ModMenuApiV2*>(getApi(MODMENU_API_VERSION_2));
    if (!api || api->struct_size < sizeof(ModMenuApiV2)) return false;

    ModMenuModInfoV2 info{};
    info.struct_size = sizeof(info);
    info.mod = mod;
    info.build_custom_screen = BuildCustomScreen;
    return api->register_mod(&info) != 0;
}
```

API V1 remains available for mods that only need the shared settings form. Use V2 to register both ordinary setting rows and a custom screen, or only the custom screen.
