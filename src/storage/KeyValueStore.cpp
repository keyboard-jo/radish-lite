#include "storage/KeyValueStore.hpp"



KeyValueStore::KeyValueStore(asio::io_context& ic) : eviction_timer_(ic) {}

KeyValueStore::~KeyValueStore() {
    clear_table(db.newer);
    clear_table(db.older);
}

void KeyValueStore::schedule_next_eviction() {
    if (ttl_queue_.empty()) return;

    eviction_timer_.expires_at(ttl_queue_.top().expiration);

    eviction_timer_.async_wait([this](std::error_code ec) {
        if (ec == asio::error::operation_aborted) return;
        
        prune_expired_keys();
    });
}

void KeyValueStore::prune_expired_keys() {
    auto now = std::chrono::steady_clock::now();

    while (!ttl_queue_.empty()) {
        auto top = ttl_queue_.top();

        if (top.expiration > now) {
            break;
        }

        ttl_queue_.pop();

        CacheEntry lookup_key;
        lookup_key.key = top.key;
        lookup_key.node.hcode = str_hash(top.key);

        HNode** existing = db.lookup(&lookup_key.node, entry_eq);
        if (existing) {
            CacheEntry* entry = reinterpret_cast<CacheEntry*>(*existing);
            
            // TODO: Might not be efficient
            if (entry->expiration.has_value() && entry->expiration.value() == top.expiration) {
                HNode* detached_node = db.detach(&lookup_key.node, entry_eq);
                delete reinterpret_cast<CacheEntry*>(detached_node);
            }
        }
    }

    schedule_next_eviction();
}

void KeyValueStore::clear_table(HTab& tab) {
    if (!tab.tab) return;
    for (size_t i = 0; i <= tab.mask; ++i) {
        HNode* cur = tab.tab[i];
        while (cur) {
            HNode* next = cur->next;
            delete reinterpret_cast<CacheEntry*>(cur);
            cur = next;
        }
    }
    tab.destroy();
}

void KeyValueStore::set(const std::string& key, const std::string& value, std::optional<std::chrono::milliseconds> ttl) {
    CacheEntry lookup_key;
    lookup_key.key = key;
    lookup_key.node.hcode = str_hash(key);

    std::optional<std::chrono::steady_clock::time_point> expire_time = std::nullopt;
    if (ttl.has_value()) {
        expire_time = std::chrono::steady_clock::now() + ttl.value();
    }

    if (HNode** existing = db.lookup(&lookup_key.node, entry_eq)) {
        CacheEntry* entry = reinterpret_cast<CacheEntry*>(*existing);
        entry->value = value;
        entry->expiration = expire_time;
    } else {
        CacheEntry* new_entry = new CacheEntry();
        new_entry->key = key;
        new_entry->value = value;
        new_entry->node.hcode = lookup_key.node.hcode;
        new_entry->expiration = expire_time;
        db.insert(&new_entry->node);
    }

    if (expire_time.has_value()) {
        ttl_queue_.push({key, expire_time.value()});
        
        if (ttl_queue_.top().key == key && ttl_queue_.top().expiration == expire_time.value()) {
            schedule_next_eviction();
        }
    }
}

std::optional<std::string> KeyValueStore::get(const std::string& key) {
    CacheEntry lookup_key;
    lookup_key.key = key;
    lookup_key.node.hcode = str_hash(key);

    HNode** from = db.lookup(&lookup_key.node, entry_eq);
    if (!from) {
        return std::nullopt;
    }

    CacheEntry* entry = reinterpret_cast<CacheEntry*>(*from);

    if (entry->expiration.has_value() && entry->expiration.value() <= std::chrono::steady_clock::now()) {
        HNode* detached_node = db.detach(&lookup_key.node, entry_eq);
        delete reinterpret_cast<CacheEntry*>(detached_node);
        return std::nullopt;
    }

    return entry->value;
}


bool KeyValueStore::remove(std::string_view key) {
    LookupEntry lookup_key;
    lookup_key.key = key;
    lookup_key.node.hcode = str_hash(key);

    // Use lookup_eq instead of entry_eq
    HNode* detached_node = db.detach(&lookup_key.node, lookup_eq);
    if (detached_node) {
        delete reinterpret_cast<CacheEntry*>(detached_node);
        return true;
    }
    return false;
}

// TODO: Defer large deletion to background worker to free memory
// NOTE: Might block the server if there are many keys
int64_t KeyValueStore::remove_batch(const std::vector<std::string_view>& keys) {
    int64_t deleted_count = 0;

    for (std::string_view key : keys) {
        if (remove(key)) {
            deleted_count++;
        }
    }
    return deleted_count;
}

bool KeyValueStore::exists(std::string_view key) {
    LookupEntry lookup_key;
    lookup_key.key = key;
    lookup_key.node.hcode = str_hash(key);

    // Use lookup_eq instead of entry_eq
    return db.lookup(&lookup_key.node, lookup_eq) != nullptr;
}

int64_t KeyValueStore::exists_batch(const std::vector<std::string_view>& keys) {
    int64_t existing_count = 0;

    for (std::string_view key : keys) {
        if (exists(key)) {
            existing_count++;
        }
    }

    return existing_count;
}