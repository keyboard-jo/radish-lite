#pragma once

#include <asio.hpp>
#include "storage/KeyValueStore.hpp"
#include "server/ServerConfig.hpp"
#include "command/CommandDispatcher.hpp"

class Server {
public:
    Server(
        asio::io_context& ioContext,
        ServerConfig config,
        KeyValueStore& store
    );

private:
    asio::awaitable<void> async_main();
    asio::awaitable<void> handle_client(asio::ip::tcp::socket socket);

    asio::ip::tcp::acceptor acceptor_;
    KeyValueStore& store_;
    ServerConfig config_;
    CommandDispatcher dispatcher_;
};