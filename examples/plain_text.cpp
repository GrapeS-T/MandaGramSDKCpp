#include "mandagram_plugin.h"
#include <cstring>

static const PluginMetadata metadata = {
    "Plain Text C++", "1.0.0", "MandaGram Community",
    "Removes the .plain prefix from outgoing text"
};

extern "C" PLUGIN_EXPORT const PluginMetadata* mandagram_plugin_metadata() {
    return &metadata;
}

// No allocation: the host copies the returned substring during this call.
static const char* before_send(int64_t, const char* text, uintptr_t) noexcept {
    if (text && std::strncmp(text, ".plain ", 7) == 0) return text + 7;
    return text;
}

extern "C" PLUGIN_EXPORT bool mandagram_plugin_init(const PluginApi* api) {
    if (!api || !api->register_message_send_hook) return false;
    api->log_info("PlainCpp", "Registering text hook");
    return api->register_message_send_hook(before_send, nullptr, 0) == 0;
}
