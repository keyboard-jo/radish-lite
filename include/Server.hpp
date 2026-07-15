#pragma once

#include <asio.hpp>
#include "KeyValueStore.hpp"

class Server {
    public:
        Server(
            asio::io_context& ioContext,
            unsigned short port,
            KeyValueStore& store
        );
    private:
        asio::awaitable<void> async_main();


        asio::ip::tcp::acceptor acceptor_;
        KeyValueStore& store_;
};
