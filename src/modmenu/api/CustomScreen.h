#pragma once

#include "modmenu/api/Settings.h"

#ifdef __cplusplus
extern "C" {
#endif

enum { MODMENU_API_VERSION_2 = 2 };

typedef struct ModMenuRectV2 {
    float x0;
    float x1;
    float y0;
    float y1;
} ModMenuRectV2;

typedef struct ModMenuScreenBuilderV2 {
    uint32_t struct_size;
    void* context;
    void (*add_label)(void* context, ModMenuRectV2 bounds, const char* text, int32_t alignment);
    void (*add_button)(void* context, ModMenuRectV2 bounds, const char* label,
                       ModMenuActionV1 action, void* user_data);
    void (*add_toggle)(void* context, ModMenuRectV2 bounds, const char* label,
                       ModMenuGetBoolV1 get, ModMenuSetBoolV1 set, void* user_data);
    void (*add_slider)(void* context, ModMenuRectV2 bounds, const char* label,
                       ModMenuGetFloatV1 get, ModMenuSetFloatV1 set, void* user_data,
                       float min_value, float max_value, float step, int32_t decimals, const char* suffix);
    void (*add_choice)(void* context, ModMenuRectV2 bounds, const char* label,
                       const char* const* options, size_t option_count,
                       ModMenuGetIntV1 get, ModMenuSetIntV1 set, void* user_data);
    void (*add_color)(void* context, ModMenuRectV2 bounds, const char* label,
                      ModMenuGetIntV1 get, ModMenuSetIntV1 set, void* user_data);
    void (*add_text)(void* context, ModMenuRectV2 bounds, const char* label,
                     ModMenuGetTextV1 get, ModMenuSetTextV1 set, void* user_data, const char* placeholder);
    void (*add_dynamic_label)(void* context, ModMenuRectV2 bounds,
                              ModMenuGetTextV1 get, void* user_data, int32_t alignment);
} ModMenuScreenBuilderV2;

typedef void (*ModMenuCustomScreenBuildV2)(void* user_data, const ModMenuScreenBuilderV2* builder,
                                           float width, float height);

typedef struct ModMenuModInfoV2 {
    uint32_t struct_size;
    ModMenuModInfoV1 mod;
    ModMenuCustomScreenBuildV2 build_custom_screen;
    void* custom_screen_user_data;
} ModMenuModInfoV2;

typedef struct ModMenuApiV2 {
    uint32_t struct_size;
    uint32_t version;
    int (*register_mod)(const ModMenuModInfoV2* info);
    void (*unregister_mod)(const char* id);
    int (*open_custom_screen)(const char* id);
} ModMenuApiV2;

#ifdef __cplusplus
}
#endif
