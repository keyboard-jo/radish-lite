#include "KeyValueStore.hpp"

void KeyValueStore::set(const std::string& key, const std::string& value, int ttlSeconds) {
    auto expiry = std::chrono::steady_clock::now() + std::chrono::seconds(ttlSeconds);
    store[key] = CacheEntry{value, expiry}; 
}

std::optional<std::string> KeyValueStore::get(const std::string& key) {
    auto it = store.find(key);

    if (it == store.end()) {
        return std::nullopt;
    }

    if (std::chrono::steady_clock::now() > it->second.expiresAt) {
        store.erase(it);
        return std::nullopt;
    }

    return it->second.value;
}

