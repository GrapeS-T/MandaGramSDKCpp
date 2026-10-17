#include "mandagram_plugin.h"
#include <atomic>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <system_error>

namespace {
constexpr char plugin_name[] = "chat_counter";
const PluginApi* host = nullptr;
std::atomic<unsigned long long> received{0};
static_assert(std::atomic<unsigned long long>::is_always_lock_free,
              "This recipe requires lock-free 64-bit atomics (x64).");
const PluginMetadata metadata = {
    "Chat Counter", "1.0.0", "MandaGram Community",
    "Counts receive callbacks without retaining message text or identifiers"
};

// Called only during init, before publishing any callback.
bool read_config(const PluginApi* api, bool& enabled) noexcept {
    try {
        char directory[4096];
        const int32_t size = api->get_storage_dir(plugin_name, directory,
                                                sizeof(directory));
        // A full buffer is ambiguous: never open a possibly truncated path.
        if (size <= 0 || size >= static_cast<int32_t>(sizeof(directory) - 1)) {
            api->log_error("ChatCounter", "Storage path unavailable or too long");
            return false;
        }
        const auto path = std::filesystem::u8path(directory) / "config.txt";
        std::error_code error;
        const bool exists = std::filesystem::exists(path, error);
        if (error) {
            api->log_error("ChatCounter", "Cannot inspect config.txt");
            return false;
        }
        enabled = true;
        if (!exists) return true;
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            api->log_error("ChatCounter", "Cannot open config.txt");
            return false;
        }
        char contents[32];
        file.read(contents, sizeof(contents));
        auto length = file.gcount();
        if (file.bad() || !file.eof() || length == sizeof(contents)) {
            api->log_error("ChatCounter", "Unreadable or oversized config.txt");
            return false;
        }
        if (length > 0 && contents[length - 1] == '\n') --length;
        if (length > 0 && contents[length - 1] == '\r') --length;
        if (length != 9 || std::memcmp(contents, "enabled=", 8) != 0 ||
            (contents[8] != '0' && contents[8] != '1')) {
            api->log_error("ChatCounter", "Expected enabled=0 or enabled=1");
            return false;
        }
        enabled = contents[8] == '1';
        return true;
    } catch (...) {
        api->log_error("ChatCounter", "Configuration initialization failed");
        return false;
    }
}

void on_receive(int64_t, int64_t, const char*, uintptr_t) noexcept {
    // No text/ID inspection, allocation, file access, API call or network request.
    received.fetch_add(1, std::memory_order_relaxed);
}

void on_click(int64_t, uintptr_t) noexcept {
    char message[96];
    const int size = std::snprintf(message, sizeof(message),
        "Receive callbacks since init: %llu",
        received.load(std::memory_order_relaxed));
    if (size > 0 && size < static_cast<int>(sizeof(message))) {
        host->log_info("ChatCounter", message);
    }
}
} // namespace

extern "C" PLUGIN_EXPORT const PluginMetadata* mandagram_plugin_metadata() noexcept {
    return &metadata;
}

extern "C" PLUGIN_EXPORT bool mandagram_plugin_init(const PluginApi* api) noexcept {
    if (!api || !api->get_storage_dir || !api->log_info || !api->log_error ||
        !api->register_message_receive_hook || !api->register_chat_action) return false;
    bool enabled = true;
    if (!read_config(api, enabled)) return false; // No registrations exist yet.
    if (!enabled) {
        api->log_info("ChatCounter", "Disabled by config.txt; no callbacks registered");
        return true;
    }
    host = api;
    received.store(0, std::memory_order_relaxed);
    const int32_t receive_status = api->register_message_receive_hook(on_receive, 0);
    const int32_t action_status = api->register_chat_action(
        "chat_counter.show_count", "Show receive count", on_click, 0);
    if (receive_status != 0 || action_status != 0) {
        api->log_error("ChatCounter", "Registration failed; disable plugin and restart client");
        // The ABI has no rollback/unregister. Returning false after registration
        // could close the native library while the host still holds callbacks.
        return true;
    }
    api->log_info("ChatCounter", "Counter and chat action registered");
    return true;
}
