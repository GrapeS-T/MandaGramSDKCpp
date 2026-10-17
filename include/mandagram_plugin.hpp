#ifndef MANDAGRAM_PLUGIN_HPP
#define MANDAGRAM_PLUGIN_HPP

#include "mandagram_plugin.h"
#include <string>
#include <vector>
#include <stdexcept>
#include <functional>
#include <memory>

namespace mandagram {

class HookContext {
public:
    explicit HookContext(HookParam* param) : param_(param) {}

    HookParam* raw() const noexcept { return param_; }

    void* thisPtr() const noexcept {
        return param_ ? param_->this_ptr : nullptr;
    }

    size_t argsCount() const noexcept {
        return param_ ? static_cast<size_t>(param_->args_count) : 0;
    }

    void* getArg(size_t index) const {
        if (!param_ || index >= param_->args_count) {
            throw std::out_of_range("Arg index out of range");
        }
        return param_->args[index];
    }

    template <typename T>
    T* getArgAs(size_t index) const {
        return static_cast<T*>(getArg(index));
    }

    void setResult(void* res) {
        if (param_) {
            param_->result = res;
            param_->has_result = 1;
        }
    }

    void returnEarly(void* res = nullptr) {
        if (param_) {
            param_->return_early = 1;
            if (res != nullptr) {
                param_->result = res;
                param_->has_result = 1;
            }
        }
    }

    void* getResult() const noexcept {
        return param_ ? param_->result : nullptr;
    }

    bool hasResult() const noexcept {
        return param_ && param_->has_result != 0;
    }

private:
    HookParam* param_;
};

class Plugin {
public:
    explicit Plugin(const PluginApi* api) : api_(api) {}
    virtual ~Plugin() = default;

    const PluginApi* rawApi() const noexcept { return api_; }

    // Core Engine
    bool registerHook(
        const std::string& className,
        const std::string& methodName,
        const std::string& methodDescriptor,
        HookBeforeFn beforeCb,
        HookAfterFn afterCb
    ) {
        if (!api_ || !api_->register_hook) return false;
        return api_->register_hook(
            className.c_str(),
            methodName.c_str(),
            methodDescriptor.empty() ? nullptr : methodDescriptor.c_str(),
            beforeCb,
            afterCb
        ) == 0;
    }

    void logInfo(const std::string& tag, const std::string& message) {
        if (api_ && api_->log_info) {
            api_->log_info(tag.c_str(), message.c_str());
        }
    }

    void logError(const std::string& tag, const std::string& message) {
        if (api_ && api_->log_error) {
            api_->log_error(tag.c_str(), message.c_str());
        }
    }

    std::string getStorageDir(const std::string& pluginName) {
        if (!api_ || !api_->get_storage_dir) return "";
        char buf[1024] = {0};
        int32_t len = api_->get_storage_dir(pluginName.c_str(), buf, sizeof(buf));
        return (len > 0) ? std::string(buf, len) : "";
    }

    std::string getCacheDir(const std::string& pluginName) {
        if (!api_ || !api_->get_cache_dir) return "";
        char buf[1024] = {0};
        int32_t len = api_->get_cache_dir(pluginName.c_str(), buf, sizeof(buf));
        return (len > 0) ? std::string(buf, len) : "";
    }

    std::string resolveAsset(const std::string& pluginName, const std::string& assetName) {
        if (!api_ || !api_->resolve_asset) return "";
        char buf[1024] = {0};
        int32_t len = api_->resolve_asset(pluginName.c_str(), assetName.c_str(), buf, sizeof(buf));
        return (len > 0) ? std::string(buf, len) : "";
    }

    int64_t getCurrentUserId() {
        if (!api_ || !api_->get_current_user_id) return 0;
        return api_->get_current_user_id();
    }

    int64_t getActiveChatId() {
        if (!api_ || !api_->get_active_chat_id) return 0;
        return api_->get_active_chat_id();
    }

    std::string executeTdlibQuery(const std::string& jsonQuery) {
        if (!api_ || !api_->execute_tdlib_query) return "";
        char buf[8192] = {0};
        int32_t len = api_->execute_tdlib_query(jsonQuery.c_str(), buf, sizeof(buf));
        return (len > 0) ? std::string(buf, len) : "";
    }

    // High-Level Event Hooks (HookRegistry)
    bool registerUpdateHook(UpdateHookFn cb, uintptr_t userData = 0) {
        if (!api_ || !api_->register_update_hook) return false;
        return api_->register_update_hook(cb, userData) == 0;
    }

    bool registerMessageSendHook(MessageBeforeSendFn beforeCb, MessageAfterSendFn afterCb = nullptr, uintptr_t userData = 0) {
        if (!api_ || !api_->register_message_send_hook) return false;
        return api_->register_message_send_hook(beforeCb, afterCb, userData) == 0;
    }

    bool registerMessageReceiveHook(MessageReceiveFn cb, uintptr_t userData = 0) {
        if (!api_ || !api_->register_message_receive_hook) return false;
        return api_->register_message_receive_hook(cb, userData) == 0;
    }

    bool registerChatAction(const std::string& id, const std::string& label, ChatActionClickFn clickCb, uintptr_t userData = 0) {
        if (!api_ || !api_->register_chat_action) return false;
        return api_->register_chat_action(id.c_str(), label.c_str(), clickCb, userData) == 0;
    }

    // Pure FFI Object & Field Reflection
    jobject getObjectField(jobject obj, const std::string& fieldName) {
        if (!api_ || !api_->get_object_field) return nullptr;
        return api_->get_object_field(obj, fieldName.c_str());
    }

    void setObjectField(jobject obj, const std::string& fieldName, jobject val) {
        if (api_ && api_->set_object_field) {
            api_->set_object_field(obj, fieldName.c_str(), val);
        }
    }

    int32_t getIntField(jobject obj, const std::string& fieldName) {
        if (!api_ || !api_->get_int_field) return 0;
        return api_->get_int_field(obj, fieldName.c_str());
    }

    void setIntField(jobject obj, const std::string& fieldName, int32_t val) {
        if (api_ && api_->set_int_field) {
            api_->set_int_field(obj, fieldName.c_str(), val);
        }
    }

    int64_t getLongField(jobject obj, const std::string& fieldName) {
        if (!api_ || !api_->get_long_field) return 0;
        return api_->get_long_field(obj, fieldName.c_str());
    }

    void setLongField(jobject obj, const std::string& fieldName, int64_t val) {
        if (api_ && api_->set_long_field) {
            api_->set_long_field(obj, fieldName.c_str(), val);
        }
    }

    bool getBoolField(jobject obj, const std::string& fieldName) {
        if (!api_ || !api_->get_bool_field) return false;
        return api_->get_bool_field(obj, fieldName.c_str());
    }

    void setBoolField(jobject obj, const std::string& fieldName, bool val) {
        if (api_ && api_->set_bool_field) {
            api_->set_bool_field(obj, fieldName.c_str(), val);
        }
    }

    std::string getStringUtf(jobject jstr) {
        if (!api_ || !api_->get_string_utf || !jstr) return "";
        char buf[2048] = {0};
        int32_t len = api_->get_string_utf(jstr, buf, sizeof(buf));
        return (len > 0) ? std::string(buf, len) : "";
    }

    jobject createStringUtf(const std::string& str) {
        if (!api_ || !api_->create_string_utf) return nullptr;
        return api_->create_string_utf(str.c_str());
    }

    jobject getAttachedField(jobject obj, const std::string& key) {
        if (!api_ || !api_->get_attached_field) return nullptr;
        return api_->get_attached_field(obj, key.c_str());
    }

    void setAttachedField(jobject obj, const std::string& key, jobject val) {
        if (api_ && api_->set_attached_field) {
            api_->set_attached_field(obj, key.c_str(), val);
        }
    }

    jobject getStateValue(jobject stateObj) {
        if (!api_ || !api_->get_state_value) return nullptr;
        return api_->get_state_value(stateObj);
    }

    void setStateValue(jobject stateObj, jobject val) {
        if (api_ && api_->set_state_value) {
            api_->set_state_value(stateObj, val);
        }
    }

    // Pure FFI Compose Modifiers
    jobject modifierEmpty() {
        if (!api_ || !api_->modifier_empty) return nullptr;
        return api_->modifier_empty();
    }

    jobject modifierSize(jobject mod, float widthDp, float heightDp) {
        if (!api_ || !api_->modifier_size) return mod;
        return api_->modifier_size(mod, widthDp, heightDp);
    }

    jobject modifierWidth(jobject mod, float widthDp) {
        if (!api_ || !api_->modifier_width) return mod;
        return api_->modifier_width(mod, widthDp);
    }

    jobject modifierHeight(jobject mod, float heightDp) {
        if (!api_ || !api_->modifier_height) return mod;
        return api_->modifier_height(mod, heightDp);
    }

    jobject modifierFillMaxWidth(jobject mod) {
        if (!api_ || !api_->modifier_fill_max_width) return mod;
        return api_->modifier_fill_max_width(mod);
    }

    jobject modifierFillMaxHeight(jobject mod) {
        if (!api_ || !api_->modifier_fill_max_height) return mod;
        return api_->modifier_fill_max_height(mod);
    }

    jobject modifierPadding(jobject mod, float startDp, float topDp, float endDp, float bottomDp) {
        if (!api_ || !api_->modifier_padding) return mod;
        return api_->modifier_padding(mod, startDp, topDp, endDp, bottomDp);
    }

    jobject modifierBackground(jobject mod, uint32_t argb, float cornerRadiusDp = 0.0f) {
        if (!api_ || !api_->modifier_background) return mod;
        return api_->modifier_background(mod, argb, cornerRadiusDp);
    }

    jobject modifierBorder(jobject mod, float widthDp, uint32_t argb, float cornerRadiusDp = 0.0f) {
        if (!api_ || !api_->modifier_border) return mod;
        return api_->modifier_border(mod, widthDp, argb, cornerRadiusDp);
    }

    jobject modifierClickable(jobject mod, jobject onClickLambda) {
        if (!api_ || !api_->modifier_clickable) return mod;
        return api_->modifier_clickable(mod, onClickLambda);
    }

    jobject modifierWeight(jobject scope, jobject mod, float weight) {
        if (!api_ || !api_->modifier_weight) return mod;
        return api_->modifier_weight(scope, mod, weight);
    }

    // Pure FFI Compose Components
    void composeRow(jobject composer, jobject mod, jobject contentLambda) {
        if (api_ && api_->compose_row) {
            api_->compose_row(composer, mod, contentLambda);
        }
    }

    void composeColumn(jobject composer, jobject mod, jobject contentLambda) {
        if (api_ && api_->compose_column) {
            api_->compose_column(composer, mod, contentLambda);
        }
    }

    void composeBox(jobject composer, jobject mod, jobject contentLambda = nullptr) {
        if (api_ && api_->compose_box) {
            api_->compose_box(composer, mod, contentLambda);
        }
    }

    void composeSpacer(jobject composer, jobject mod) {
        if (api_ && api_->compose_spacer) {
            api_->compose_spacer(composer, mod);
        }
    }

    void composeText(jobject composer, const std::string& text, jobject mod, uint32_t argb, float fontSizeSp = 14.0f, bool isBold = false, int32_t maxLines = 1) {
        if (api_ && api_->compose_text) {
            api_->compose_text(composer, text.c_str(), mod, argb, fontSizeSp, isBold, maxLines);
        }
    }

    void composeImage(jobject composer, jobject bitmap, jobject mod) {
        if (api_ && api_->compose_image) {
            api_->compose_image(composer, bitmap, mod);
        }
    }

    jobject getAvatarBitmap(const std::string& path) {
        if (!api_ || !api_->get_avatar_bitmap) return nullptr;
        return api_->get_avatar_bitmap(path.c_str());
    }

private:
    const PluginApi* api_;
};

}

#define MANDAGRAM_DECLARE_METADATA(NAME, VERSION, AUTHOR, DESCRIPTION) \
    extern "C" PLUGIN_EXPORT const PluginMetadata* mandagram_plugin_metadata(void) { \
        static const PluginMetadata meta = { \
            NAME, \
            VERSION, \
            AUTHOR, \
            DESCRIPTION \
        }; \
        return &meta; \
    }

#endif
