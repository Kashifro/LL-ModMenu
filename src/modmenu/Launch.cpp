#include "modmenu/internal/Launch.hpp"

#include "ll/api/service/TargetedBedrock.h"
#include "mc/deps/application/AppPlatform.h"
#include "platform/Log.hpp"

#include <Windows.h>
#include <shellapi.h>

namespace modmenu::detail {

void OpenUri(const std::string& uri) {
    if (!uri.starts_with("https://") && !uri.starts_with("http://")) {
        Log().warn("modmenu: refusing to open non-web URI");
        return;
    }
    auto platform = ll::service::getAppPlatform();
    if (!platform || !platform->canLaunchUri(uri)) {
        Log().error("modmenu: could not open URI");
        return;
    }
    platform->launchUri(uri);
}

void OpenFolder(const std::filesystem::path& folder) {
    const auto result = ::ShellExecuteW(nullptr, L"open", folder.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result) <= 32)
        Log().error("modmenu: could not open folder {} (ShellExecuteW error {})", folder.string(),
                    reinterpret_cast<INT_PTR>(result));
}

}
