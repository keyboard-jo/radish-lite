#include <catch2/catch_test_macros.hpp>
#include "protocol/RespParser.hpp"
#include "protocol/RespValue.hpp"

TEST_CASE("RespParser parses Simple Strings correctly", "[RespParser]") 
{
    auto result = RespParser::parse("+OK\r\n");
    
    REQUIRE(result.value.has_value());
    CHECK(result.value->type == RespType::SimpleString);
    CHECK(result.value->string == "OK");
    CHECK(result.consumed == 5);
}

TEST_CASE("RespParser parses Errors correctly", "[RespParser]") 
{
    auto result = RespParser::parse("-ERR unknown command 'FOO'\r\n");
    
    REQUIRE(result.value.has_value());
    CHECK(result.value->type == RespType::Error);
    CHECK(result.value->string == "ERR unknown command 'FOO'");
    CHECK(result.consumed == 28);
}

TEST_CASE("RespParser parses Integers correctly", "[RespParser]") 
{
    SECTION("Positive integer") 
    {
        auto result = RespParser::parse(":1000\r\n");
        
        REQUIRE(result.value.has_value());
        CHECK(result.value->type == RespType::Integer);
        CHECK(result.value->integer == 1000);
        CHECK(result.consumed == 7);
    }

    SECTION("Negative integer") 
    {
        auto result = RespParser::parse(":-42\r\n");
        
        REQUIRE(result.value.has_value());
        CHECK(result.value->type == RespType::Integer);
        CHECK(result.value->integer == -42);
        CHECK(result.consumed == 6);
    }
}

TEST_CASE("RespParser parses Bulk Strings correctly", "[RespParser]") 
{
    SECTION("Standard bulk string") 
    {
        auto result = RespParser::parse("$5\r\nhello\r\n");
        
        REQUIRE(result.value.has_value());
        CHECK(result.value->type == RespType::BulkString);
        CHECK(result.value->string == "hello");
        CHECK(result.consumed == 11);
    }

    SECTION("Empty bulk string") 
    {
        auto result = RespParser::parse("$0\r\n\r\n");
        
        REQUIRE(result.value.has_value());
        CHECK(result.value->type == RespType::BulkString);
        CHECK(result.value->string.empty());
        CHECK(result.consumed == 6);
    }
}

TEST_CASE("RespParser parses Arrays correctly", "[RespParser]") 
{
    // Command equivalent to: ["ECHO", "HELLO"]
    auto result = RespParser::parse("*2\r\n$4\r\nECHO\r\n$5\r\nHELLO\r\n");
    
    REQUIRE(result.value.has_value());
    CHECK(result.value->type == RespType::Array);
    REQUIRE(result.value->array.size() == 2);
    
    CHECK(result.value->array[0].type == RespType::BulkString);
    CHECK(result.value->array[0].string == "ECHO");
    
    CHECK(result.value->array[1].type == RespType::BulkString);
    CHECK(result.value->array[1].string == "HELLO");
    
    CHECK(result.consumed == 25);
}

TEST_CASE("RespParser handles incomplete data (partial reads)", "[RespParser]") 
{
    SECTION("Missing CRLF for simple string") 
    {
        auto result = RespParser::parse("+OK");
        
        CHECK_FALSE(result.value.has_value());
        CHECK(result.consumed == 0);
    }

    SECTION("Incomplete bulk string payload") 
    {
        auto result = RespParser::parse("$5\r\nhel");
        
        CHECK_FALSE(result.value.has_value());
        CHECK(result.consumed == 0);
    }

    SECTION("Incomplete array contents") 
    {
        // Array header says 2 items, but only 1 item is provided
        auto result = RespParser::parse("*2\r\n$4\r\nECHO\r\n");
        
        CHECK_FALSE(result.value.has_value());
        CHECK(result.consumed == 0);
    }
}

TEST_CASE("RespParser throws exceptions on malformed data", "[RespParser]") 
{
    SECTION("Unknown RESP type identifier") 
    {
        CHECK_THROWS_AS(RespParser::parse("?invalid\r\n"), std::runtime_error);
    }

    SECTION("Malformed integer format") 
    {
        CHECK_THROWS_AS(RespParser::parse(":not_an_int\r\n"), std::runtime_error);
    }
}