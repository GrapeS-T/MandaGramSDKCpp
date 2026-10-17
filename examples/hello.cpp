#include "mandagram_plugin.h"

static const PluginMetadata meta = {
    "Hello Plugin", "1.0.0", "Dev", "Minimal native plugin"
};

extern "C" PLUGIN_EXPORT const PluginMetadata* mandagram_plugin_metadata() {
    return &meta;
}

extern "C" PLUGIN_EXPORT bool mandagram_plugin_init(const PluginApi* api) {
    if (!api) return false;
    api->log_info("HelloPlugin", "Plugin initialized");
    return true;
}
