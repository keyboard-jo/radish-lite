#include "protocol/RespParser.hpp"

#include <stdexcept>

using asio::ip::tcp;

asio::awaitable<RespValue> RespParser::parse(
    tcp::socket& socket)
{
    co_return co_await parse_value(socket);
}

std::string_view RespParser::unread() const noexcept
{
    return std::string_view(
        buffer_.data() + cursor_,
        buffer_.size() - cursor_
    );
}

void RespParser::consume(std::size_t bytes)
{
    cursor_ += bytes;

    if (cursor_ >= COMPACT_THRESHOLD)
    {
        compact();
    }
}

void RespParser::compact()
{
    if (cursor_ < COMPACT_THRESHOLD)
        return;

    if (cursor_ < buffer_.size() / 2)
        return;

    buffer_.erase(0, cursor_);
    cursor_ = 0;
}

asio::awaitable<void> RespParser::read_until_crlf(
    tcp::socket& socket)
{
    co_await asio::async_read_until(
        socket,
        asio::dynamic_buffer(buffer_),
        "\r\n",
        asio::use_awaitable);
}

asio::awaitable<void> RespParser::ensure_bytes(
    asio::ip::tcp::socket& socket,
    std::size_t bytes)
{
    std::size_t available =
        buffer_.size() - cursor_;

    if (available >= bytes)
    {
        co_return;
    }

    co_await asio::async_read(
        socket,
        asio::dynamic_buffer(buffer_),
        asio::transfer_exactly(
            bytes - available),
        asio::use_awaitable);
}

std::int64_t RespParser::parse_integer(
    std::string_view text)
{
    bool negative = false;
    std::size_t i = 0;

    if (!text.empty() && text.front() == '-')
    {
        negative = true;
        i = 1;
    }

    std::int64_t value = 0;

    for (; i < text.size(); ++i)
    {
        char c = text[i];

        if (c < '0' || c > '9')
            throw std::runtime_error("Invalid integer.");

        value = value * 10 + (c - '0');
    }

    return negative ? -value : value;
}

asio::awaitable<RespValue> RespParser::parse_value(
    tcp::socket& socket)
{
    co_await read_until_crlf(socket);

    switch (unread().front())
    {
    case '+':
        co_return co_await parse_simple_string(socket);

    case '-':
        co_return co_await parse_error(socket);

    case ':':
        co_return co_await parse_integer_value(socket);

    case '$':
        co_return co_await parse_bulk_string(socket);

    case '*':
        co_return co_await parse_array(socket);

    default:
        throw std::runtime_error("Unknown RESP type.");
    }
}

asio::awaitable<RespValue> RespParser::parse_simple_string(
    tcp::socket& socket)
{
    co_await read_until_crlf(socket);

    auto line = unread();
    auto cr = line.find("\r\n");

    RespValue value;
    value.type = RespType::SimpleString;
    value.string = std::string(line.substr(1, cr - 1));

    consume(cr + 2);

    co_return value;
}

asio::awaitable<RespValue> RespParser::parse_error(
    tcp::socket& socket)
{
    co_await read_until_crlf(socket);

    auto line = unread();
    auto cr = line.find("\r\n");

    RespValue value;
    value.type = RespType::Error;
    value.string = std::string(line.substr(1, cr - 1));

    consume(cr + 2);

    co_return value;
}

asio::awaitable<RespValue> RespParser::parse_integer_value(
    tcp::socket& socket)
{
    co_await read_until_crlf(socket);

    auto line = unread();
    auto cr = line.find("\r\n");

    RespValue value;
    value.type = RespType::Integer;
    value.integer = parse_integer(
        line.substr(1, cr - 1));

    consume(cr + 2);

    co_return value;
}

asio::awaitable<RespValue> RespParser::parse_bulk_string(
    tcp::socket& socket)
{
    co_await read_until_crlf(socket);

    auto line = unread();
    auto cr = line.find("\r\n");

    auto length = static_cast<std::size_t>(
        parse_integer(line.substr(1, cr - 1)));

    consume(cr + 2);

    co_await ensure_bytes(socket, length + 2);

    RespValue value;
    value.type = RespType::BulkString;
    value.string.assign(buffer_.data(), length);

    consume(length + 2);

    co_return value;
}

asio::awaitable<RespValue> RespParser::parse_array(
    tcp::socket& socket)
{
    co_await read_until_crlf(socket);

    auto line = unread();
    auto cr = line.find("\r\n");

    auto count = static_cast<std::size_t>(
        parse_integer(line.substr(1, cr - 1)));

    consume(cr + 2);

    RespValue value;
    value.type = RespType::Array;
    value.array.reserve(count);

    for (std::size_t i = 0; i < count; ++i)
    {
        value.array.push_back(
            co_await parse_value(socket));
    }

    co_return value;
}