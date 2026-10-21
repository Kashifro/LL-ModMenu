

#pragma once

#include <functional>
#include <string>

namespace nugget::keybinds {

using Handler = std::function<void()>;


void Register(const std::string& name, int def, Handler onPress);

int  Get(const std::string& name, int def);    
void Set(const std::string& name, int code);   



bool Dispatch(int code);


std::string Conflict(int code);

} 
