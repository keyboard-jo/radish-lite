#pragma once
#include <string>
#include <unordered_map>
#include <queue>
#include <chrono>
#include <optional>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <atomic>
#include <condition_variable>


struct CacheEntry {
    std::string value;
    std::chrono::time_point<std::chrono::steady_clock> expiresAt;
};

struct ExpiryNode {
    std::chrono::time_point<std::chrono::steady_clock> expiry;
    std::string key;

    bool operator>(const ExpiryNode& other) const {
        return expiry > other.expiry;
    }
};

class KeyValueStore {
    private:
        std::unordered_map<std::string, CacheEntry> store;
        std::priority_queue<ExpiryNode, std::vector<ExpiryNode>, std::greater<ExpiryNode>> pq;
        std::shared_mutex store_mtx;

        std::thread worker;
        std::atomic<bool> running;
        std::condition_variable cv;
        std::mutex cv_mutex;

        void pruneLoop();

    public:
        KeyValueStore();
        ~KeyValueStore();

        static constexpr int NO_EXPIRE = -1;
        static constexpr int PRUNE_LOOP_WAIT_TIME_MINUTES = 1;

        void set(const std::string& key, const std::string& value, int ttlSeconds = NO_EXPIRE);

        std::optional<std::string> get(const std::string& key);

        std::optional<std::string> peek(const std::string& key);

        void prune();
};