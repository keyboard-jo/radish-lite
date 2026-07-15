#include "server/Connection.hpp"

#include <asio.hpp>
#include <iostream>

asio::awaitable<void> handle_client(
    asio::ip::tcp::socket socket,
    KeyValueStore& store)
{
    asio::streambuf buffer;

    try
    {
        for (;;)
        {
            co_await asio::async_read_until(
                socket,
                buffer,
                '\n',
                asio::use_awaitable);

            // Parse request
            // Execute using store

            co_await asio::async_write(
                socket,
                buffer,
                asio::use_awaitable);
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Connection with client lost: " << e.what() << '\n';
    }

    co_return;
}