#include "protocol/RespParser.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>

using asio::ip::tcp;

RespParser::RespParser(std::size_t initial_capacity)
    : buffer_(initial_capacity)
{
}

asio::awaitable<RespValue> RespParser::parse(
    tcp::socket& socket)
{
    co_return co_await parse_value(socket);
}

std::string_view RespParser::unread() const noexcept
{
    return std::string_view(
        buffer_.data() + read_idx_,
        write_idx_ - read_idx_
    );
}

void RespParser::consume(std::size_t bytes)
{
    read_idx_ += bytes;

    // Fast-path reset: If all bytes are consumed, reset indices to zero instantly.
    if (read_idx_ == write_idx_)
    {
        read_idx_ = 0;
        write_idx_ = 0;
    }
}

void RespParser::ensure_space(std::size_t required_bytes)
{
    // 1. If we already have enough free space at the tail, do nothing.
    if (buffer_.size() - write_idx_ >= required_bytes)
    {
        return;
    }

    std::size_t unread_len = write_idx_ - read_idx_;

    // 2. Shift unread bytes to index 0 if read_idx_ > 0
    if (read_idx_ > 0)
    {
        if (unread_len > 0)
        {
            std::memmove(buffer_.data(), buffer_.data() + read_idx_, unread_len);
        }
        read_idx_ = 0;
        write_idx_ = unread_len;
    }

    // 3. If capacity is still insufficient after shift, grow the vector.
    if (buffer_.size() - write_idx_ < required_bytes)
    {
        std::size_t new_capacity = std::max(buffer_.size() * 2, write_idx_ + required_bytes);
        buffer_.resize(new_capacity);
    }
}

asio::awaitable<void> RespParser::read_until_crlf(
    tcp::socket& socket)
{
    while (true)
    {
        std::string_view view = unread();
        if (view.find("\r\n") != std::string_view::npos)
        {
            co_return; // \r\n already present in unread window!
        }

        // Ensure space to receive at least DEFAULT_READ_CHUNK bytes from network
        ensure_space(DEFAULT_READ_CHUNK);

        std::size_t bytes_read = co_await socket.async_read_some(
            asio::buffer(buffer_.data() + write_idx_, buffer_.size() - write_idx_),
            asio::use_awaitable
        );

        if (bytes_read == 0)
        {
            throw std::runtime_error("Connection closed by peer while reading CRLF.");
        }

        write_idx_ += bytes_read;
    }
}

asio::awaitable<void> RespParser::ensure_bytes(
    tcp::socket& socket,
    std::size_t bytes)
{
    while (write_idx_ - read_idx_ < bytes)
    {
        std::size_t needed = bytes - (write_idx_ - read_idx_);
        ensure_space(needed);

        std::size_t bytes_read = co_await socket.async_read_some(
            asio::buffer(buffer_.data() + write_idx_, buffer_.size() - write_idx_),
            asio::use_awaitable
        );

        if (bytes_read == 0)
        {
            throw std::runtime_error("Connection closed by peer while fetching bulk data.");
        }

        write_idx_ += bytes_read;
    }
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
    value.string.assign(unread().data(), length);

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