#include <catch2/catch_test_macros.hpp>
#include <asio.hpp>
#include <thread>
#include <chrono>

#include <storage/KeyValueStore.hpp>

TEST_CASE("KeyValueStore basic operations", "[kvstore]") {
    asio::io_context io;
    KeyValueStore store(io);

    SECTION("Set and Get values") {
        store.set("apple", "red", std::nullopt);
        store.set("banana", "yellow", std::nullopt);

        REQUIRE(store.get("apple") == "red");
        REQUIRE(store.get("banana") == "yellow");
        REQUIRE(store.get("nonexistent") == std::nullopt);
    }

    SECTION("Overwrite existing keys") {
        store.set("color", "blue", std::nullopt);
        REQUIRE(store.get("color") == "blue");

        store.set("color", "green", std::nullopt);
        REQUIRE(store.get("color") == "green");
    }

    SECTION("Remove keys") {
        store.set("key1", "val1", std::nullopt);
        REQUIRE(store.get("key1") == "val1");

        REQUIRE(store.remove("key1") == true);
        REQUIRE(store.get("key1") == std::nullopt);
        REQUIRE(store.remove("key1") == false);
    }
}

TEST_CASE("KeyValueStore TTL and expiration", "[kvstore][ttl]") {
    asio::io_context io;
    KeyValueStore store(io);

    SECTION("Key expires after TTL") {
        store.set("temp_key", "temp_val", std::chrono::milliseconds(50));

        // Should exist immediately
        REQUIRE(store.get("temp_key") == "temp_val");

        // Run the io_context in a separate thread or poll it after waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        io.poll(); // Dispatches the timer callback / pruning

        // Should be expired and removed
        REQUIRE(store.get("temp_key") == std::nullopt);
    }

    SECTION("Multiple keys with different TTLs") {
        store.set("short", "1", std::chrono::milliseconds(30));
        store.set("long", "2", std::chrono::milliseconds(200));

        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        io.poll();

        REQUIRE(store.get("short") == std::nullopt);
        REQUIRE(store.get("long") == "2");
    }
}