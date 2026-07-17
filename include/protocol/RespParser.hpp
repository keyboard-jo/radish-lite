#pragma once

#include <asio.hpp>

#include "protocol/RespValue.hpp"

class RespParser
{
public:
    asio::awaitable<RespValue> parse(
        asio::ip::tcp::socket& socket);

private:
    asio::streambuf buffer_;

    asio::awaitable<std::string> read_line(
        asio::ip::tcp::socket& socket);

    asio::awaitable<std::string> read_bulk_string(
        asio::ip::tcp::socket& socket);
};