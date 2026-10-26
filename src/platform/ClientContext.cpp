#include "platform/ClientContext.hpp"

#include "ll/api/service/TargetedBedrock.h"

namespace modmenu::clientctx {

namespace {
std::string screenName;
ClientInstance* captured = nullptr;
}

ClientInstance* Client() {
    if (auto* ci = ll::service::getClientInstance().as_ptr()) return ci;
    return captured;
}

void Capture(ClientInstance* client) { captured = client; }

void SetScreenName(const std::string& name) {
    if (name == "toast_screen" || name == "debug_screen" || name == "hud_crosshair_screen") return;
    screenName = name;
}

bool IsOnScreen(const std::string& name) { return screenName == name; }

}
