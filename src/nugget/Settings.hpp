
#pragma once

#include <string>

namespace nugget::settings {

bool GetBool(const std::string& key, bool def);
void SetBool(const std::string& key, bool value);
std::string GetString(const std::string& key, const std::string& def);
void SetString(const std::string& key, const std::string& value);
int  GetInt(const std::string& key, int def);
void SetInt(const std::string& key, int value);



void SetImmediateWrites(bool enabled);
void Flush();

} 
