#pragma once

#include <string>

namespace modmenu::prefs {

bool GetBool(const std::string& key, bool def);
void SetBool(const std::string& key, bool value);
std::string GetString(const std::string& key, const std::string& def);
void SetString(const std::string& key, const std::string& value);
int  GetInt(const std::string& key, int def);
void SetInt(const std::string& key, int value);


}
