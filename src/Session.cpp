#include "Session.hpp"

#include <iostream>
#include <string>

Session::Session(
    asio::ip::tcp::socket socket,
    KeyValueStore& store)
    : socket_(std::move(socket)),
      store_(store)
{
}

void Session::start() {
    doRead();
}

void Session::doRead() {
    auto self = shared_from_this();

    socket_.async_read_some(
        asio::buffer(buffer_),

        [this, self](std::error_code ec, std::size_t length) {
            if (ec) {
                std::cout << "Client disconnected: " << ec.message() << '\n';
                return;
            }

            std::string message(buffer_.data(), length);

            std::cout << "Received: " << message << '\n';

            // Continue reading from this client.
            doRead();
        });
}

void Session::doWrite() {
    auto self = shared_from_this();

    asio::async_write(
        socket_,
        asio::buffer(response_),

        [this, self](std::error_code ec, std::size_t /*length*/) {
            if (ec) {
                std::cout << "Write failed: " << ec.message() << '\n';
                return;
            }

            // After writing, wait for the next command.
            doRead();
        });
}