#ifndef MANDAGRAM_PLUGIN_H
#define MANDAGRAM_PLUGIN_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* jobject;

typedef struct HookParam {
    void* this_ptr;
    void** args;
    uint32_t args_count;
    uint32_t padding;
    void* result;
    uint8_t return_early;
    uint8_t has_result;
    uint16_t reserved;
    uint32_t padding2;
} HookParam;

typedef void (*HookBeforeFn)(HookParam* param);
typedef void (*HookAfterFn)(HookParam* param);

typedef struct PluginMetadata {
    const char* name;
    const char* version;
    const char* author;
    const char* description;
} PluginMetadata;

typedef const PluginMetadata* (*PluginMetadataFn)(void);

typedef void (*ComposableFn)(jobject composer, uintptr_t user_data);
typedef void (*ScopedComposableFn)(jobject scope, jobject composer, uintptr_t user_data);

typedef bool (*UpdateHookFn)(const char* update_json, uintptr_t user_data);
typedef const char* (*MessageBeforeSendFn)(int64_t chat_id, const char* text, uintptr_t user_data);
typedef void (*MessageAfterSendFn)(int64_t chat_id, int64_t message_id, uintptr_t user_data);
typedef void (*MessageReceiveFn)(int64_t chat_id, int64_t message_id, const char* text, uintptr_t user_data);
typedef void (*ChatActionClickFn)(int64_t chat_id, uintptr_t user_data);

typedef struct PluginApi {
    /* 0..8: Core Engine */
    int32_t (*register_hook)(
        const char* class_name,
        const char* method_name,
        const char* method_descriptor,
        HookBeforeFn before_cb,
        HookAfterFn after_cb
    );
    void (*log_info)(const char* tag, const char* msg);
    void (*log_error)(const char* tag, const char* msg);
    int32_t (*get_storage_dir)(const char* plugin_name, char* out_buf, uint32_t max_len);
    int32_t (*get_cache_dir)(const char* plugin_name, char* out_buf, uint32_t max_len);
    int32_t (*resolve_asset)(const char* plugin_name, const char* asset_name, char* out_buf, uint32_t max_len);
    int64_t (*get_current_user_id)(void);
    int64_t (*get_active_chat_id)(void);
    int32_t (*execute_tdlib_query)(const char* json_query, char* out_buf, uint32_t max_len);

    /* 9..16: Low-level Compose Runtime Primitives */
    jobject (*create_composable_lambda)(ComposableFn cb, uintptr_t user_data);
    jobject (*create_scoped_lambda)(ScopedComposableFn cb, uintptr_t user_data);
    void (*replace_composable)(HookParam* param, ComposableFn cb, uintptr_t user_data);
    void (*composer_start_group)(jobject composer, int32_t key);
    void (*composer_end_group)(jobject composer);
    jobject (*composer_get_remembered)(jobject composer);
    void (*composer_set_remembered)(jobject composer, jobject val);
    bool (*composer_is_skipping)(jobject composer);

    /* 17..20: High-Level Event Hooks (HookRegistry) */
    int32_t (*register_update_hook)(UpdateHookFn cb, uintptr_t user_data);
    int32_t (*register_message_send_hook)(MessageBeforeSendFn before_cb, MessageAfterSendFn after_cb, uintptr_t user_data);
    int32_t (*register_message_receive_hook)(MessageReceiveFn cb, uintptr_t user_data);
    int32_t (*register_chat_action)(const char* id, const char* label, ChatActionClickFn click_cb, uintptr_t user_data);

    /* 21..34: Pure FFI Object & Field Reflection */
    jobject (*get_object_field)(jobject obj, const char* field_name);
    void (*set_object_field)(jobject obj, const char* field_name, jobject val);
    int32_t (*get_int_field)(jobject obj, const char* field_name);
    void (*set_int_field)(jobject obj, const char* field_name, int32_t val);
    int64_t (*get_long_field)(jobject obj, const char* field_name);
    void (*set_long_field)(jobject obj, const char* field_name, int64_t val);
    bool (*get_bool_field)(jobject obj, const char* field_name);
    void (*set_bool_field)(jobject obj, const char* field_name, bool val);
    int32_t (*get_string_utf)(jobject jstr, char* out_buf, uint32_t max_len);
    jobject (*create_string_utf)(const char* str);
    jobject (*get_attached_field)(jobject obj, const char* key);
    void (*set_attached_field)(jobject obj, const char* key, jobject val);
    jobject (*get_state_value)(jobject state_obj);
    void (*set_state_value)(jobject state_obj, jobject val);

    /* 35..45: Pure FFI Compose Modifiers */
    jobject (*modifier_empty)(void);
    jobject (*modifier_size)(jobject mod, float width_dp, float height_dp);
    jobject (*modifier_width)(jobject mod, float width_dp);
    jobject (*modifier_height)(jobject mod, float height_dp);
    jobject (*modifier_fill_max_width)(jobject mod);
    jobject (*modifier_fill_max_height)(jobject mod);
    jobject (*modifier_padding)(jobject mod, float start_dp, float top_dp, float end_dp, float bottom_dp);
    jobject (*modifier_background)(jobject mod, uint32_t argb, float corner_radius_dp);
    jobject (*modifier_border)(jobject mod, float width_dp, uint32_t argb, float corner_radius_dp);
    jobject (*modifier_clickable)(jobject mod, jobject on_click_lambda);
    jobject (*modifier_weight)(jobject scope, jobject mod, float weight);

    /* 46..52: Pure FFI Compose Components */
    void (*compose_row)(jobject composer, jobject mod, jobject content_lambda);
    void (*compose_column)(jobject composer, jobject mod, jobject content_lambda);
    void (*compose_box)(jobject composer, jobject mod, jobject content_lambda);
    void (*compose_spacer)(jobject composer, jobject mod);
    void (*compose_text)(jobject composer, const char* text, jobject mod, uint32_t argb, float font_size_sp, bool is_bold, int32_t max_lines);
    void (*compose_image)(jobject composer, jobject bitmap, jobject mod);
    jobject (*get_avatar_bitmap)(const char* path);
} PluginApi;

#ifdef _WIN32
#define PLUGIN_EXPORT __declspec(dllexport)
#else
#define PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

typedef bool (*PluginInitFn)(const PluginApi* api);

#ifdef __cplusplus
}
#endif

#endif
