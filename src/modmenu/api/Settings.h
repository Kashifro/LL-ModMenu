#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { MODMENU_API_VERSION_1 = 1 };

typedef enum ModMenuSettingKindV1 {
    MODMENU_SETTING_TOGGLE = 0,
    MODMENU_SETTING_KEY = 1,
    MODMENU_SETTING_SLIDER = 2,
    MODMENU_SETTING_SECTION = 3,
    MODMENU_SETTING_COLOR = 4,
    MODMENU_SETTING_CATEGORY = 5,
    MODMENU_SETTING_ACTION = 6,
    MODMENU_SETTING_CHOICE = 7,
    MODMENU_SETTING_TEXT = 8
} ModMenuSettingKindV1;

typedef uint8_t (*ModMenuGetBoolV1)(void* user_data);
typedef void (*ModMenuSetBoolV1)(void* user_data, uint8_t value);
typedef int32_t (*ModMenuGetIntV1)(void* user_data);
typedef void (*ModMenuSetIntV1)(void* user_data, int32_t value);
typedef float (*ModMenuGetFloatV1)(void* user_data);
typedef void (*ModMenuSetFloatV1)(void* user_data, float value);
typedef const char* (*ModMenuGetTextV1)(void* user_data);
typedef void (*ModMenuSetTextV1)(void* user_data, const char* value);
typedef void (*ModMenuActionV1)(void* user_data);
typedef uint8_t (*ModMenuShownIfV1)(void* user_data);

typedef struct ModMenuSettingV1 {
    uint32_t struct_size;
    ModMenuSettingKindV1 kind;
    const char* label;
    const char* tooltip;
    void* user_data;
    ModMenuGetBoolV1 get_bool;
    ModMenuSetBoolV1 set_bool;
    ModMenuGetIntV1 get_int;
    ModMenuSetIntV1 set_int;
    ModMenuGetFloatV1 get_float;
    ModMenuSetFloatV1 set_float;
    ModMenuGetTextV1 get_text;
    ModMenuSetTextV1 set_text;
    ModMenuActionV1 action;
    ModMenuShownIfV1 shown_if;
    uint8_t default_bool;
    int32_t default_int;
    float default_float;
    float min_value;
    float max_value;
    float step;
    int32_t decimals;
    uint8_t confirm_action;
    const char* suffix;
    const char* default_text;
    const char* placeholder;
    const char* glyph;
    const char* const* options;
    size_t option_count;
    const struct ModMenuSettingV1* children;
    size_t child_count;
} ModMenuSettingV1;

typedef struct ModMenuModInfoV1 {
    uint32_t struct_size;
    const char* id;
    const char* name;
    const char* description;
    const char* version;
    const char* author;
    const char* icon;
    const char* website;
    const char* issues;
    const char* source;
    const char* license;
    int32_t quick_config_key;
    const ModMenuSettingV1* settings;
    size_t setting_count;
} ModMenuModInfoV1;

typedef struct ModMenuApiV1 {
    uint32_t struct_size;
    uint32_t version;
    int (*register_mod)(const ModMenuModInfoV1* info);
    void (*unregister_mod)(const char* id);
} ModMenuApiV1;

#ifdef __cplusplus
}
#endif
