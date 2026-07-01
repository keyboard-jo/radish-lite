#include <catch2/catch_test_macros.hpp>
#include "KeyValueStore.hpp"
#include <thread>
#include <chrono>

TEST_CASE("KeyValueStore basic set and get", "[KeyValueStore]") {
    KeyValueStore kvs;

    kvs.set("key1", "value1", 100);
    auto val = kvs.get("key1");

    REQUIRE(val.has_value());
    REQUIRE(val.value() == "value1");
}

TEST_CASE("KeyValueStore get non-existent key", "[KeyValueStore]") {
    KeyValueStore kvs;
    auto val = kvs.get("nonexistent");

    REQUIRE_FALSE(val.has_value());
}

TEST_CASE("KeyValueStore expiration", "[KeyValueStore]") {
    KeyValueStore kvs;

    kvs.set("key_temp", "value_temp", 1); // 1 second TTL
    
    // Immediately should exist
    REQUIRE(kvs.get("key_temp").has_value());

    // Wait for expiration
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    // Should not exist
    REQUIRE_FALSE(kvs.get("key_temp").has_value());
}

TEST_CASE("KeyValueStore overwrite key", "[KeyValueStore]") {
    KeyValueStore kvs;

    kvs.set("key1", "val1", 100);
    kvs.set("key1", "val2", 100);

    auto val = kvs.get("key1");
    REQUIRE(val.has_value());
    REQUIRE(val.value() == "val2");
}

TEST_CASE("KeyValueStore prune", "[KeyValueStore]") {
    KeyValueStore kvs;

    kvs.set("key1", "val1", 1);
    kvs.set("key2", "val2", 100);

    // Wait for key1 to expire
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    kvs.prune();

    REQUIRE_FALSE(kvs.get("key1").has_value());
    REQUIRE(kvs.get("key2").has_value());
}
