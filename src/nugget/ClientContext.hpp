
#pragma once

#include <string>

class ClientInstance;

namespace nugget::clientctx {

ClientInstance* Client();   
void Capture(ClientInstance* client);   

void SetScreenName(const std::string& name);
bool IsOnScreen(const std::string& name);

} 
