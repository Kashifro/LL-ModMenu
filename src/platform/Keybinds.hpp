#pragma once

#include <string>

namespace modmenu::keybinds {

// LL-ModMenu's one rebindable key  Shown in Settings > Controls.
void Register(const std::string& name, int def);
int Get();
void Set(int code);
bool IsOpenKey(int code);

// Which other Controls action already uses `code` ("" if none).
std::string Conflict(int code);

}
