#include "nugget/Keybinds.hpp"

#include "MossUI/core/Keys.hpp"
#include "nugget/ClientContext.hpp"
#include "nugget/Settings.hpp"

#include "ll/api/input/KeyRegistry.h"
#include "ll/api/mod/NativeMod.h"

#include "mc/client/game/ClientInstance.h"
#include "mc/client/input/KeyboardRemappingLayout.h"
#include "mc/client/input/Keymapping.h"
#include "mc/client/options/IOptionRegistry.h"

#include <vector>

namespace nugget::keybinds {
namespace {

using MossUI::BaseKey;

struct Entry { std::string name, action; int def; Handler fn; };   
std::vector<Entry> entries;

const std::string* ActionOf(const std::string& name) {   
    for (auto it = entries.rbegin(); it != entries.rend(); ++it)
        if (it->name == name) return &it->action;
    return nullptr;
}

std::string Saved(const std::string& name) { return "quickkey." + name; }

std::shared_ptr<KeyboardRemappingLayout> Layout() {
    auto* client = clientctx::Client();
    return client ? client->getOptions().getCurrentKeyboardRemapping() : nullptr;
}

const Keymapping* Find(KeyboardRemappingLayout& layout, const std::string& action) {
    for (const auto& map : *layout.mKeymappings)
        if (*map.mAction == action) return &map;
    return nullptr;
}

std::string OurPrefix() { return "key." + ll::mod::NativeMod::current()->getName() + "."; }

} 

void Register(const std::string& name, int def, Handler onPress) {
    auto& key = ll::input::KeyRegistry::getInstance().getOrCreateKey(name, {BaseKey(def)});
    
    entries.push_back({name, "key." + key.getFullName(), def, std::move(onPress)});

    if (const int saved = settings::GetInt(Saved(name), 0); saved && saved != Get(name, def)) Set(name, saved);
}

int Get(const std::string& name, int def) {
    if (auto layout = Layout(); layout)
        if (const auto* action = ActionOf(name))
            if (const auto* map = Find(*layout, *action); map && map->isAssigned() && !(*map->mKeys).empty())
                return (*map->mKeys).front();
    return BaseKey(def);
}

void Set(const std::string& name, int code) {
    code = BaseKey(code);
    settings::SetInt(Saved(name), code);
    if (auto layout = Layout(); layout)
        if (const auto* action = ActionOf(name)) layout->setMapping(*action, {code});
}

bool Dispatch(int code) {
    code = BaseKey(code);
    for (const auto& e : entries) {
        if (BaseKey(Get(e.name, e.def)) != code) continue;
        if (e.fn) e.fn();
        return true;
    }
    return false;
}

std::string Conflict(int code) {
    code = BaseKey(code);
    auto layout = Layout();
    if (!code || !layout) return {};
    const std::string ours = OurPrefix();
    for (const auto& map : *layout->mKeymappings) {
        const std::string& action = *map.mAction;
        if (!map.isAssigned() || action.starts_with(ours)) continue;
        for (int key : *map.mKeys)
            if (key == code) return "Controls: " + action.substr(action.starts_with("key.") ? 4 : 0);
    }
    return {};
}

} 
