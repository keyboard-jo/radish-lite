#include "server/Server.hpp"
#include "server/Connection.hpp"

#include <iostream>
#include <memory>

Server::Server(
    asio::io_context& ioContext,
    unsigned short port,
    KeyValueStore& store)
    : 
    acceptor_(
        ioContext,
        asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)
    ),
    store_(store) 
{
    asio::co_spawn(
        ioContext, 
        async_main(), 
        asio::detached
    );
}


asio::awaitable<void> Server::async_main() {
    auto executor = co_await asio::this_coro::executor;

    for (;;) {
        try {
            auto socket = co_await acceptor_.async_accept(asio::use_awaitable);
            
            asio::co_spawn(
                executor, 
                handle_client(std::move(socket), store_), 
                asio::detached
            );
        }
        catch (const std::exception& e) {
            std::cerr << "Accept failed: " << e.what() << '\n';
        }
    }

    co_return;
}