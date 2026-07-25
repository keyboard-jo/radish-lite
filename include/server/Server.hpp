#pragma once

#include <asio.hpp>
#include "storage/KeyValueStore.hpp"

class Server {
public:
    Server(
        asio::io_context& ioContext,
        unsigned short port,
        KeyValueStore& store
    );

private:
    asio::awaitable<void> async_main();
    asio::awaitable<void> handle_client(asio::ip::tcp::socket socket);

    asio::ip::tcp::acceptor acceptor_;
    KeyValueStore& store_;
};