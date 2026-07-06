#include "Server.hpp"
#include "Session.hpp"

#include <iostream>
#include <memory>

Server::Server(
    asio::io_context& ioContext,
    unsigned short port,
    KeyValueStore& store)
    : acceptor_(
        ioContext,
        asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
    store_(store) {
    doAccept();
}


void Server::doAccept() {
    acceptor_.async_accept(
        [this] (std::error_code ec, asio::ip::tcp::socket socket) {
            if (!ec) {
                std::cout << "Client connected\n";

                std::make_shared<Session>(
                    std::move(socket),
                    store_
                )->start();
            }



            doAccept();
        }
    );
}