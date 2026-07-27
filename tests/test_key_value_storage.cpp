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

    SECTION("Remove keys (single and batch)") {
        store.set("key1", "val1", std::nullopt);
        store.set("key2", "val2", std::nullopt);
        store.set("key3", "val3", std::nullopt);

        REQUIRE(store.get("key1") == "val1");
        REQUIRE(store.get("key2") == "val2");
        REQUIRE(store.get("key3") == "val3");

        // Test batch removal with a mix of existing and non-existing keys
        std::vector<std::string_view> batch_to_remove = {"key1", "key2", "nonexistent"};
        REQUIRE(store.remove_batch(batch_to_remove) == 2);

        // Verify they are actually gone
        REQUIRE(store.get("key1") == std::nullopt);
        REQUIRE(store.get("get2") == std::nullopt);
        
        // key3 should still exist
        REQUIRE(store.get("key3") == "val3");
        
        // Clean up remaining key
        REQUIRE(store.remove("key3") == true);
        REQUIRE(store.remove("key3") == false);
    }

    SECTION("Check keys exist (single and batch)") {
        store.set("key1", "val1", std::nullopt);
        store.set("key2", "val2", std::nullopt);

        REQUIRE(store.exists("key1") == true);
        REQUIRE(store.exists("key2") == true);
        REQUIRE(store.exists("nonexistent") == false);

        // Test batch existence check
        std::vector<std::string_view> batch_to_check = {"key1", "key2", "nonexistent1", "nonexistent2"};
        REQUIRE(store.exists_batch(batch_to_check) == 2);
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