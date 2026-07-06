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
        void doAccept();

        asio::ip::tcp::acceptor acceptor_;
        KeyValueStore& store_;
};
