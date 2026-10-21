#pragma once

#include "modmenu/api/CustomScreen.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(MODMENU_BUILD)
#define MODMENU_EXPORT __declspec(dllexport)
#else
#define MODMENU_EXPORT
#endif

MODMENU_EXPORT const void* ModMenu_GetApi(uint32_t requested_version);

#undef MODMENU_EXPORT

#ifdef __cplusplus
}
#endif
