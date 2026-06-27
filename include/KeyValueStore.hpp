#pragma once
#include <string>
#include <unordered_map>
#include <chrono>
#include <optional>


struct CacheEntry {
    std::string value;
    std::chrono::time_point<std::chrono::steady_clock> expiresAt;
};

class KeyValueStore {
    private:
        std::unordered_map<std::string, CacheEntry> store;

    public:
        void set(const std::string& key, const std::string& value, int ttlSeconds);

        std::optional<std::string> get(const std::string& key);
};