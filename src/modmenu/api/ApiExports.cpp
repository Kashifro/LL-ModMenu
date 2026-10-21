#include "modmenu/api/ApiInternal.hpp"

namespace {

int RegisterV1(const ModMenuModInfoV1* info) {
    return nugget::modmenu::api::RegisterProvider(info) ? 1 : 0;
}

int RegisterV2(const ModMenuModInfoV2* info) {
    if (!info || info->struct_size < sizeof(ModMenuModInfoV2)) return 0;
    return nugget::modmenu::api::RegisterProvider(
               &info->mod, info->build_custom_screen, info->custom_screen_user_data)
        ? 1
        : 0;
}

int OpenCustomScreen(const char* id) {
    return nugget::modmenu::api::OpenCustomScreen(id) ? 1 : 0;
}

const ModMenuApiV1 apiV1 = {
    sizeof(ModMenuApiV1), MODMENU_API_VERSION_1, RegisterV1, nugget::modmenu::api::UnregisterProvider
};

const ModMenuApiV2 apiV2 = {
    sizeof(ModMenuApiV2), MODMENU_API_VERSION_2, RegisterV2,
    nugget::modmenu::api::UnregisterProvider, OpenCustomScreen
};

}

extern "C" const void* ModMenu_GetApi(uint32_t version) {
    if (version == MODMENU_API_VERSION_1) return &apiV1;
    if (version == MODMENU_API_VERSION_2) return &apiV2;
    return nullptr;
}
