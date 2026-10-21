#pragma once

#include "modmenu/api/ModMenuAPI.h"

#include <atomic>
#include <memory>
#include <string>
#include <utility>

namespace nugget::modmenu::api {

inline std::string CopyText(const char* value) { return value ? value : ""; }

using ActiveToken = std::shared_ptr<std::atomic_bool>;

struct Registration {
    ActiveToken active;
    ModMenuCustomScreenBuildV2 buildCustomScreen = nullptr;
    void* customScreenUserData = nullptr;
    std::string name;
};

Registration FindRegistration(const std::string& id);
void StoreRegistration(const std::string& id, Registration registration);
void EraseRegistration(const std::string& id);
bool RegisterProvider(const ModMenuModInfoV1* info, ModMenuCustomScreenBuildV2 build = nullptr,
                      void* customScreenUserData = nullptr);
void UnregisterProvider(const char* id);
bool OpenCustomScreen(const char* id);

}
