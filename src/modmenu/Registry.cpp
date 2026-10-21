#include "modmenu/internal/Registry.hpp"

#include "ll/api/mod/NativeMod.h"
#include "ll/api/service/TargetedBedrock.h"
#include "mc/deps/application/AppPlatform.h"
#include "nugget/Log.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <shellapi.h>

#pragma comment(lib, "Shell32.lib")

#include <cctype>
#include <system_error>

namespace nugget::modmenu::detail {

std::string NormName(const std::string& name) {
    std::string out;
    for (unsigned char c : name) {
        if (std::isalnum(c)) out.push_back(static_cast<char>(std::tolower(c)));
    }
    return out;
}

std::vector<ModInfo>& Registry() {
    static std::vector<ModInfo> mods;
    return mods;
}

int IndexOf(const std::string& id) {
    const auto& mods = Registry();
    for (size_t i = 0; i < mods.size(); ++i)
        if (mods[i].id == id) return static_cast<int>(i);
    return -1;
}

std::string SelfVersion() {
    const auto& version = ll::mod::NativeMod::current()->getManifest().version;
    return version ? version->to_string() : std::string("1.0.0");
}

std::string ResolveModIcon(const std::filesystem::path& modDir, const std::string& icon) {
    if (icon.rfind("textures/", 0) == 0) return icon;
    const std::filesystem::path relative(icon);
    if (relative.empty() || relative.is_absolute() || relative.has_root_name() || relative.has_parent_path() ||
        relative.extension() != ".png")
        return {};

    std::error_code ec;
    const auto file = modDir / relative;
    if (!std::filesystem::is_regular_file(file, ec) || ec) return {};
    const auto canonicalDir = std::filesystem::weakly_canonical(modDir, ec);
    if (ec) return {};
    const auto canonicalFile = std::filesystem::canonical(file, ec);
    if (ec || canonicalFile.parent_path() != canonicalDir) return {};
    return canonicalFile.generic_string();
}

void OpenUri(const std::string& uri) {
    if (uri.rfind("https://", 0) != 0 && uri.rfind("http://", 0) != 0) {
        nugget::Log().warn("modmenu: refusing to open non-web URI");
        return;
    }
    auto platform = ll::service::getAppPlatform();
    if (!platform || !platform->canLaunchUri(uri)) {
        nugget::Log().error("modmenu: could not open URI");
        return;
    }
    platform->launchUri(uri);
}

void OpenFolder(const std::filesystem::path& folder) {
    std::error_code ec;
    if (!std::filesystem::is_directory(folder, ec) || ec) {
        nugget::Log().error("modmenu: folder does not exist: {}", folder.string());
        return;
    }
    const auto absoluteFolder = std::filesystem::absolute(folder, ec);
    if (ec) {
        nugget::Log().error("modmenu: could not resolve folder {}: {}", folder.string(), ec.message());
        return;
    }
    const auto result = ::ShellExecuteW(nullptr, L"open", absoluteFolder.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result) <= 32)
        nugget::Log().error("modmenu: could not open folder {} (ShellExecuteW error {})",
                            absoluteFolder.string(), reinterpret_cast<INT_PTR>(result));
}

}
