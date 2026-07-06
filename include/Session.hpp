#pragma once

#include <array>
#include <memory>

#include <asio.hpp>

#include "KeyValueStore.hpp"


class Session : public std::enable_shared_from_this<Session> {
    public:
        Session(
            asio::ip::tcp::socket socket,
            KeyValueStore& store);
        
        void start();

    private:
        void doRead();
        void doWrite();

        asio::ip::tcp::socket socket_;
        KeyValueStore& store_;

        std::array<char, 1024> buffer_;
        std::string response_;
};