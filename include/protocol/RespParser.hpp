#pragma once

#include <asio.hpp>

#include <cstdint>
#include <string>
#include <string_view>

#include "protocol/RespValue.hpp"

class RespParser
{
public:
    explicit RespParser(std::size_t initial_capacity = 8192); 

    asio::awaitable<RespValue> parse(
        asio::ip::tcp::socket& socket);

private:
    static constexpr std::size_t DEFAULT_READ_CHUNK = 1024;

    std::vector<char> buffer_;
    std::size_t read_idx_ = 0;
    std::size_t write_idx_ = 0;

    //
    // Buffer management
    //
    void ensure_space(std::size_t required_bytes);

    asio::awaitable<void> read_until_crlf(
        asio::ip::tcp::socket& socket);

    asio::awaitable<void> ensure_bytes(
        asio::ip::tcp::socket& socket,
        std::size_t bytes);

    [[nodiscard]]
    std::string_view unread() const noexcept;

    void consume(std::size_t bytes);

    //
    // Parsing helpers
    //
    [[nodiscard]]
    static std::int64_t parse_integer(
        std::string_view text);

    //
    // RESP parsers
    //
    asio::awaitable<RespValue> parse_value(
        asio::ip::tcp::socket& socket);

    asio::awaitable<RespValue> parse_simple_string(
        asio::ip::tcp::socket& socket);

    asio::awaitable<RespValue> parse_error(
        asio::ip::tcp::socket& socket);

    asio::awaitable<RespValue> parse_integer_value(
        asio::ip::tcp::socket& socket);

    asio::awaitable<RespValue> parse_bulk_string(
        asio::ip::tcp::socket& socket);

    asio::awaitable<RespValue> parse_array(
        asio::ip::tcp::socket& socket);
};