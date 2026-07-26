#include "server/Server.hpp"
#include "server/ServerConfig.hpp"
#include "protocol/RespParser.hpp"
#include "command/CommandDispatcher.hpp"

#include <iostream>
#include <memory>
#include <vector>
#include <cstring>

using asio::ip::tcp;

Server::Server(
    asio::io_context& ioContext,
    ServerConfig config,
    KeyValueStore& store)
    : 
    acceptor_(ioContext, config),
    store_(store),
    config_(std::move(config)),
    dispatcher_(store_)
{
    asio::co_spawn(
        ioContext, 
        async_main(), 
        asio::detached
    );
}

void print_resp(const RespValue& val, int depth = 0) {
    std::string indent(depth * 2, ' ');
    switch (val.type)
    {
        case RespType::SimpleString:
            std::cout << indent << "[SimpleString] " << val.string << "\n";
            break;
        case RespType::Error:
            std::cout << indent << "[Error] " << val.string << "\n";
            break;
        case RespType::Integer:
            std::cout << indent << "[Integer] " << val.integer << "\n";
            break;
        case RespType::BulkString:
            std::cout << indent << "[BulkString] \"" << val.string << "\"\n";
            break;
        case RespType::Array:
            std::cout << indent << "[Array] size = " << val.array.size() << "\n";
            for (const auto& elem : val.array) {
                print_resp(elem, depth + 1);
            }
            break;
    }
}

asio::awaitable<void> Server::handle_client(tcp::socket socket) {
    // Moved buffer responsibility directly into the connection handler
    std::vector<char> buffer(8192);
    std::size_t read_idx = 0;
    std::size_t write_idx = 0;

    try
    {
        for (;;)
        {
            // 1. Try to parse from whatever data we currently have
            std::string_view unread_view(buffer.data() + read_idx, write_idx - read_idx);
            auto result = RespParser::parse(unread_view);

            if (result.value.has_value())
            {
                // A complete RESP value was successfully parsed!
                read_idx += result.consumed;

                // Fast-path reset to keep buffer clean
                if (read_idx == write_idx)
                {
                    read_idx = 0;
                    write_idx = 0;
                }

                RespValue request = std::move(*result.value);

                print_resp(request);

                std::string response = dispatcher_.dispatch(request);

                co_await asio::async_write(
                    socket,
                    asio::buffer(response),
                    asio::use_awaitable);
            }
            else
            {
                // We need more data from the network to form a complete RESP message.
                
                // If we've hit the end of our vector capacity, shift or grow
                if (write_idx == buffer.size())
                {
                    if (read_idx > 0)
                    {
                        std::size_t unread_len = write_idx - read_idx;
                        std::memmove(buffer.data(), buffer.data() + read_idx, unread_len);
                        read_idx = 0;
                        write_idx = unread_len;
                    }
                    else
                    {
                        if (buffer.size() >= config_.max_buffer_size) {
                            throw std::runtime_error("Request exceeded maximum allowed size limit.");
                        }
                        std::size_t new_capacity = std::min(buffer.size() * 2, config_.max_buffer_size);

                        buffer.resize(new_capacity);
                    }
                }

                // 2. Await network read
                std::size_t bytes_read = co_await socket.async_read_some(
                    asio::buffer(buffer.data() + write_idx, buffer.size() - write_idx),
                    asio::use_awaitable
                );

                if (bytes_read == 0)
                {
                    throw std::runtime_error("Connection closed by peer.");
                }

                write_idx += bytes_read;
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Connection with client lost: " << e.what() << '\n';
    }

    co_return;
}


asio::awaitable<void> Server::async_main() {
    auto executor = co_await asio::this_coro::executor;

    for (;;) {
        try {
            auto socket = co_await acceptor_.async_accept(asio::use_awaitable);
            
            asio::co_spawn(
                executor, 
                handle_client(std::move(socket)), 
                asio::detached
            );
        }
        catch (const std::exception& e) {
            std::cerr << "Accept failed: " << e.what() << '\n';
        }
    }

    co_return;
}