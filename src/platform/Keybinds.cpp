#include "platform/Keybinds.hpp"

#include "MossUI/core/Keys.hpp"
#include "platform/ClientContext.hpp"
#include "platform/Log.hpp"
#include "platform/Prefs.hpp"

#include "ll/api/input/KeyRegistry.h"

#include "mc/client/game/ClientInstance.h"
#include "mc/client/input/KeyboardRemappingLayout.h"
#include "mc/client/input/Keymapping.h"
#include "mc/client/options/IOptionRegistry.h"

#include <memory>

namespace modmenu::keybinds {
namespace {

using MossUI::BaseKey;

constexpr const char* SAVED_KEY = "modmenu.open_key";

std::string action; // the Controls action of the open key
int defaultKey = 0;

std::shared_ptr<KeyboardRemappingLayout> Layout() {
    auto* client = clientctx::Client();
    return client ? client->getOptions().getCurrentKeyboardRemapping() : nullptr;
}

const Keymapping* Find(KeyboardRemappingLayout& layout) {
    for (const auto& map : *layout.mKeymappings)
        if (*map.mAction == action) return &map;
    return nullptr;
}

}

void Register(const std::string& name, int def) {
    defaultKey = def;
    action = "key." + ll::input::KeyRegistry::getInstance().getOrCreateKey(name, {BaseKey(def)}).getFullName();
    if (const int saved = prefs::GetInt(SAVED_KEY, 0); saved && saved != Get()) Set(saved);
}

int Get() {
    if (auto layout = Layout(); layout)
        if (const auto* map = Find(*layout); map && map->isAssigned() && !(*map->mKeys).empty())
            return (*map->mKeys).front();
    return BaseKey(defaultKey);
}

void Set(int code) {
    code = BaseKey(code);
    prefs::SetInt(SAVED_KEY, code);
    if (auto layout = Layout(); layout) layout->setMapping(action, {code});
}

bool IsOpenKey(int code) { return BaseKey(Get()) == BaseKey(code); }

std::string Conflict(int code) {
    code = BaseKey(code);
    auto layout = Layout();
    if (!code || !layout) return {};
    const std::string ours = "key." + Self().getName() + ".";
    for (const auto& map : *layout->mKeymappings) {
        const std::string& name = *map.mAction;
        if (!map.isAssigned() || name.starts_with(ours)) continue;
        for (int key : *map.mKeys)
            if (key == code) return "Controls: " + name.substr(name.starts_with("key.") ? 4 : 0);
    }
    return {};
}

}
