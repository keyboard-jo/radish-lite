#pragma once

#include <asio.hpp>
#include <string>
#include <optional>
#include <chrono>
#include <queue>
#include <cstdint>
#include <cassert>
#include <cstdlib>


namespace KVConfig {
    constexpr size_t kMaxLoadFactor = 8;
    constexpr size_t kRehashingWork = 128;
    constexpr uint64_t kFnvOffsetBasis = 0xcbf29ce484222325;
    constexpr uint64_t kFnvPrime = 0x100000001b3;
}

struct HNode {
    HNode* next = nullptr;
    uint64_t hcode = 0;
};

struct CacheEntry {
    HNode node;
    std::string key;
    std::string value;
    std::optional<std::chrono::steady_clock::time_point> expiration = std::nullopt;
};

inline bool entry_eq(HNode* lhs, HNode* rhs) {
    CacheEntry* le = reinterpret_cast<struct CacheEntry*>(lhs);
    CacheEntry* re = reinterpret_cast<struct CacheEntry*>(rhs);
    return le->key == re->key;
}

inline uint64_t str_hash(std::string_view data) {
    uint64_t hash = KVConfig::kFnvOffsetBasis;
    for (char c : data) {
        hash ^= static_cast<uint8_t>(c);
        hash *= KVConfig::kFnvPrime;
    }

    return hash;
}


struct LookupEntry {
    HNode node;
    std::string_view key;
};

inline bool lookup_eq(HNode* lhs, HNode* rhs) {
    CacheEntry* le = reinterpret_cast<CacheEntry*>(lhs);
    LookupEntry* re = reinterpret_cast<LookupEntry*>(rhs);
    return le->key == re->key;
}

struct HTab
{
    HNode** tab = nullptr;
    size_t mask = 0;
    size_t size = 0;

    void init(size_t n) {
        assert(n > 0 && ((n - 1) & n) == 0);
        tab = (HNode**)std::calloc(n, sizeof(HNode*));
        mask = n - 1;
        size = 0;
    }

    void insert(HNode* node) {
        size_t pos = node->hcode & mask;
        node->next = tab[pos];
        tab[pos] = node;
        size++;
    }

    HNode** lookup(HNode* key, bool (*eq)(HNode*, HNode*)) {
        if (!tab) return nullptr;
        size_t pos = key->hcode & mask;
        HNode** from = &tab[pos];

        for (HNode* cur; (cur = *from) != nullptr; from = &cur->next) {
            if (cur->hcode == key->hcode && eq(cur, key)) {
                return from;
            }
        }

        return nullptr;
    }

    HNode* detach(HNode** from) {
        HNode* node = *from;
        *from = node->next;
        size--;
        return node;
    }

    void destroy() {
        if (tab) {
            std::free(tab);
            tab = nullptr;
        }
    }
};

struct HMap {
    HTab newer;
    HTab older;
    size_t migrate_pos;

    void help_rehashing() {
        size_t nwork = 0;
        while (nwork < KVConfig::kRehashingWork && older.size > 0) {
            HNode** from = &older.tab[migrate_pos];
            if (!*from) {
                migrate_pos++;
                continue;
            }
            
            newer.insert(older.detach(from));
            nwork++;
        }

        if (older.size == 0 && older.tab) {
            older.destroy();
            older = HTab{};
        }
    }

    void trigger_rehashing() {
        older = newer;
        newer.init((newer.mask + 1) * 2);
        migrate_pos = 0;
    }

    void insert(HNode* node) {
        if (!newer.tab) {
            newer.init(4);
        }

        if(!older.tab) {
            size_t threshold = (newer.mask + 1) * KVConfig::kMaxLoadFactor;
            if (newer.size >= threshold) {
                trigger_rehashing();
            }
        }
        help_rehashing();

        newer.insert(node);
    }

    HNode** lookup(HNode* key, bool (*eq)(HNode*, HNode*)) {
        help_rehashing();
        HNode** from = newer.lookup(key, eq);
        if (!from) {
            from = older.lookup(key, eq); 
        }
        return from;
    }

    HNode* detach(HNode* key, bool (*eq)(HNode*, HNode*)) {
        help_rehashing();
        if (HNode** from = newer.lookup(key, eq)) {
            return newer.detach(from);
        }
        if (HNode** from = older.lookup(key, eq)) {
            return older.detach(from);
        }
        return nullptr;
    }
};

struct TTLRecord {
    std::string key;
    std::chrono::steady_clock::time_point expiration;

    bool operator<(const TTLRecord& other) const {
        return expiration > other.expiration;
    }
};


class KeyValueStore {
private:
    HMap db;
    asio::steady_timer eviction_timer_;
    std::priority_queue<TTLRecord> ttl_queue_;

    void clear_table(HTab& tab);

public:
    KeyValueStore(asio::io_context& io);
    ~KeyValueStore();

    void schedule_next_eviction();
    void prune_expired_keys();

    void set(const std::string& key, const std::string& value, std::optional<std::chrono::milliseconds> ttl);
    std::optional<std::string> get(const std::string& key);
    bool remove(std::string_view key);
    int64_t remove_batch(const std::vector<std::string_view>& keys);
    bool exists(std::string_view key);
    int64_t exists_batch(const std::vector<std::string_view>& keys);
};