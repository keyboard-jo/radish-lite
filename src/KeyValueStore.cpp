#include "KeyValueStore.hpp"

namespace {
    static constexpr int PRUNE_LOOP_WAIT_TIME_MINUTES = 1;
}

KeyValueStore::KeyValueStore()
    : running(true) {
        worker = std::thread(&KeyValueStore::pruneLoop, this);
    }

KeyValueStore::~KeyValueStore() {
    running = false;
    cv.notify_all();

    if (worker.joinable()) {
        worker.join();
    }
}

void KeyValueStore::pruneLoop() {
    while (running) {
        std::unique_lock<std::mutex> lock(cv_mutex);

        cv.wait_for(
            lock, 
            std::chrono::minutes(PRUNE_LOOP_WAIT_TIME_MINUTES), 
            [this] { return !running; }
        );

        if (running) {
            prune();
        }
    }
}

void KeyValueStore::set(const std::string& key, const std::string& value, int ttlSeconds) {
    std::unique_lock<std::shared_mutex> lock(store_mtx);

    auto expiry = std::chrono::steady_clock::now() + std::chrono::seconds(ttlSeconds);
    store[key] = CacheEntry{value, expiry}; 

    pq.push({expiry, key});
}

std::optional<std::string> KeyValueStore::get(const std::string& key) {
    std::shared_lock<std::shared_mutex> lock(store_mtx);

    auto it = store.find(key);

    if (it == store.end()) {
        return std::nullopt;
    }

    if (std::chrono::steady_clock::now() > it->second.expiresAt) {
        // Skip, handle by background worker
        return std::nullopt;
    }

    return it->second.value;
}

void KeyValueStore::prune() {
    std::unique_lock<std::shared_mutex> lock(store_mtx);
    auto now = std::chrono::steady_clock::now();

    while (!pq.empty() && pq.top().expiry <= now) {
        const auto& node = pq.top();
        auto it = store.find(node.key);

        if (it != store.end() && it->second.expiresAt <= now) {
            store.erase(it);
        }

        pq.pop();
    }
}