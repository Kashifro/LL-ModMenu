
#include "hooks/Hooks.hpp"

#include "MossUI/MossUI.hpp"
#include "modmenu/ModMenu.hpp"
#include "nugget/ClientContext.hpp"
#include "nugget/Log.hpp"

#include "ll/api/memory/Hook.h"
#include "ll/api/mod/NativeMod.h"
#include "ll/api/service/Bedrock.h"

#include <cstdint>
#include <algorithm>
#include <exception>
#include <filesystem>
#include <memory>
#include <string_view>
#include <utility>

#include "mc/client/game/ClientInstance.h"
#include "mc/client/gui/DirtyFlag.h"
#include "mc/client/gui/ViewRequest.h"
#include "mc/client/gui/controls/UIPropertyBag.h"
#include "mc/client/gui/screens/ScreenController.h"
#include "mc/client/gui/screens/controllers/StartMenuScreenController.h"
#include "mc/deps/core/resource/PackIdVersion.h"
#include "mc/deps/core/resource/PackType.h"
#include "mc/resources/IRepositoryFactory.h"
#include "mc/resources/ResourcePackRepository.h"

namespace {

constexpr uint32_t Fnv1a32(std::string_view s) {
    uint32_t h = 0x811C9DC5u;
    for (char c : s) h = (h ^ static_cast<uint8_t>(c)) * 0x01000193u;
    return h;
}

constexpr uint32_t MODS_BUTTON_ID = Fnv1a32("button.mods_button_clicked");

void* wired = nullptr;
bool resourcePackPathRegistered = false;
bool resourcePackEnabled = false;
bool resourcePackLookupLogged = false;
bool resourceRepositoryWaitLogged = false;
bool resourceManifestMissingLogged = false;
bool resourcePackHookInstalled = false;
std::filesystem::path resourcePackDir;

void RegisterResourcePackPath(ResourcePackRepository& repo) {
    if (resourcePackPathRegistered) return;
    const auto manifestPath = resourcePackDir / "ModMenu" / "manifest.json";
    if (!std::filesystem::exists(manifestPath)) {
        if (!resourceManifestMissingLogged) {
            nugget::Log().error("LL-ModMenu resource-pack manifest is missing: {}", manifestPath.string());
            resourceManifestMissingLogged = true;
        }
        return;
    }

    repo.addCustomResourcePackPath(resourcePackDir, PackType::Resources);
    repo.refreshPacks();
    resourcePackPathRegistered = true;
    nugget::Log().info("Registered LL-ModMenu resource-pack search path {}", resourcePackDir.string());
}

void EnsureModResourcePack() {
    auto repository = ll::service::getResourcePackRepository(true);
    if (!repository) {
        if (!resourceRepositoryWaitLogged) {
            nugget::Log().warn("Client resource-pack repository is not available yet");
            resourceRepositoryWaitLogged = true;
        }
        return;
    }

    auto& repo = *repository;
    RegisterResourcePackPath(repo);
    if (resourcePackEnabled) return;

    auto identity = PackIdVersion::fromString("0f88edb7-4218-4fca-8a48-9b826cac27b4_0.1.0");
    identity.mPackType = PackType::Resources;
    if (!repo.getResourcePackForPackId(identity)) {
        if (!resourcePackLookupLogged) {
            nugget::Log().warn("LL-ModMenu resource pack is not in the resource repository yet");
            resourcePackLookupLogged = true;
        }
        return;
    }

    auto servicePacks = *repo.mServicePacks;
    if (std::find(servicePacks.begin(), servicePacks.end(), identity) == servicePacks.end()) {
        servicePacks.push_back(identity);
        nugget::Log().info("Adding LL-ModMenu resource pack to the service-pack stack");
        if (!repo.setServicePacks(std::move(servicePacks))) {
            nugget::Log().error("LeviLamina rejected the LL-ModMenu resource pack service-pack entry");
            return;
        }
    }

    resourcePackEnabled = true;
    nugget::Log().info("Enabled LL-ModMenu resource pack in the service-pack stack");
}

void WireModsButton(::StartMenuScreenController* c) {
    EnsureModResourcePack();
    if (wired == c) return;
    wired = c;
    c->registerButtonPressedHandler(MODS_BUTTON_ID, [](::UIPropertyBag*) -> ::ui::ViewRequest {
        if (!MossUI::IsOpen()) {
            try {
                nugget::modmenu::Open();
            } catch (const std::exception& e) {
                nugget::Log().error("Mods button failed: {}", e.what());
            } catch (...) {
                nugget::Log().error("Mods button failed");
            }
        }
        return static_cast<::ui::ViewRequest>(
            static_cast<uint>(::ui::ViewRequest::ConsumeEvent) | static_cast<uint>(::ui::ViewRequest::DelayedFocusRefresh));
    });
}


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
LL_TYPE_INSTANCE_HOOK(
    ClientInstanceUpdateHook,
    ll::memory::HookPriority::Normal,
    ClientInstance,
    &ClientInstance::$update,
    bool,
    bool isInitFinished
) {
    EnsureModResourcePack();
    nugget::clientctx::Capture(this);
    return origin(isInitFinished);
}

LL_TYPE_INSTANCE_HOOK(
    ClientInstanceGrabHook,
    ll::memory::HookPriority::Normal,
    ClientInstance,
    &ClientInstance::$grabMouse,
    void
) {
    if (MossUI::SuppressMouseGrab()) return;
    origin();
}

LL_TYPE_INSTANCE_HOOK(
    ResourcePackRepositoryCtorHook,
    ll::memory::HookPriority::Normal,
    ResourcePackRepository,
    &ResourcePackRepository::$ctor,
    void*,
    ::gsl::not_null<::std::shared_ptr<::RepositoryPacks>> repositoryPacks,
    ::PackManifestFactory& manifestFactory,
    ::Bedrock::NotNullNonOwnerPtr<::IContentAccessibilityProvider> const& contentAccessibility,
    ::Bedrock::NotNullNonOwnerPtr<::Core::FilePathManager> const& pathManager,
    ::Bedrock::NonOwnerPointer<::PackCommand::IPackCommandPipeline> commands,
    ::PackSourceFactory& packSourceFactory,
    ::Bedrock::NonOwnerPointer<::IMinecraftEventing> minecraftEventing,
    bool initAsync,
    ::std::unique_ptr<::IRepositoryFactory> factory
) {
    void* ret = origin(std::move(repositoryPacks), manifestFactory, contentAccessibility, pathManager,
                       std::move(commands), packSourceFactory, std::move(minecraftEventing), initAsync,
                       std::move(factory));
    RegisterResourcePackPath(*static_cast<ResourcePackRepository*>(ret));
    return ret;
}

LL_TYPE_INSTANCE_HOOK(
    StartMenuTickHook,
    ll::memory::HookPriority::Normal,
    StartMenuScreenController,
    &StartMenuScreenController::$tick,
    ::ui::DirtyFlag
) {
    const auto dirty = origin();
    WireModsButton(this);
    return dirty;
}

LL_TYPE_INSTANCE_HOOK(
    StartMenuCtorHook,
    ll::memory::HookPriority::Normal,
    StartMenuScreenController,
    &StartMenuScreenController::$ctor,
    void*,
    ::std::shared_ptr<::MainMenuScreenModel>              model,
    ::Bedrock::NotNullNonOwnerPtr<::IEntitlementManager>  entitlements
) {
    void* ret = origin(std::move(model), std::move(entitlements));
    wired = nullptr;
    return ret;
}
#pragma clang diagnostic pop

bool installed = false;

} 

namespace nugget::hooks {

void InstallResourcePackHook(const std::filesystem::path& resourceDir) {
    resourcePackDir = resourceDir;
    if (resourcePackHookInstalled) return;
    ResourcePackRepositoryCtorHook::hook();
    resourcePackHookInstalled = true;
    nugget::Log().info("Installed early resource repository hook for {}", resourcePackDir.string());
    EnsureModResourcePack();
}

void InstallClientHooks() {
    if (installed) return;
    installed = true;
    ClientInstanceUpdateHook::hook();
    ClientInstanceGrabHook::hook();
    StartMenuCtorHook::hook();
    StartMenuTickHook::hook();
    EnsureModResourcePack();
}

void RemoveClientHooks() {
    if (!installed) return;
    installed = false;
    StartMenuTickHook::unhook();
    StartMenuCtorHook::unhook();
    ClientInstanceGrabHook::unhook();
    ClientInstanceUpdateHook::unhook();
    wired = nullptr;
}

} 
