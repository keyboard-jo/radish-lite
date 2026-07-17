#include "protocol/RespParser.hpp"

#include <istream>
#include <stdexcept>

asio::awaitable<std::string> RespParser::read_line(
    asio::ip::tcp::socket& socket)
{
    co_await asio::async_read_until(
        socket,
        buffer_,
        "\r\n",
        asio::use_awaitable);

    std::istream stream(&buffer_);

    std::string line;
    std::getline(stream, line);

    // Remove '\r' left behind by getline().
    if (!line.empty() && line.back() == '\r')
    {
        line.pop_back();
    }

    co_return line;
}

asio::awaitable<std::string> RespParser::read_bulk_string(
    asio::ip::tcp::socket& socket)
{
    std::string header = co_await read_line(socket);

    if (header.empty() || header.front() != '$')
    {
        throw std::runtime_error("Expected bulk string.");
    }

    std::size_t length = std::stoul(header.substr(1));

    // Ensure we have payload + CRLF.
    co_await asio::async_read(
        socket,
        buffer_,
        asio::transfer_exactly(length + 2 - buffer_.size()),
        asio::use_awaitable);

    std::istream stream(&buffer_);

    std::string value(length, '\0');
    stream.read(value.data(), static_cast<std::streamsize>(length));

    // Consume trailing "\r\n".
    stream.get();
    stream.get();

    co_return value;
}

asio::awaitable<RespValue> RespParser::parse(
    asio::ip::tcp::socket& socket)
{
    std::string header = co_await read_line(socket);

    if (header.empty() || header.front() != '*')
    {
        throw std::runtime_error("Expected array.");
    }

    std::size_t count = std::stoul(header.substr(1));

    RespValue request;
    request.elements.reserve(count);

    for (std::size_t i = 0; i < count; ++i)
    {
        request.elements.push_back(
            co_await read_bulk_string(socket));
    }

    co_return request;
}