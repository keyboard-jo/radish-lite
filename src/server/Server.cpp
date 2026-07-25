#include "server/Server.hpp"
#include "protocol/RespParser.hpp"

#include <iostream>
#include <memory>

using asio::ip::tcp;

Server::Server(
    asio::io_context& ioContext,
    unsigned short port,
    KeyValueStore& store)
    : 
    acceptor_(
        ioContext,
        tcp::endpoint(tcp::v4(), port)
    ),
    store_(store) 
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
    // 1. Instantiate the parser here so its buffer outlives a single request.
    RespParser parser;

    try
    {
        for (;;)
        {
            // 2. Await the parsed RESP value. The parser handles all the 
            // async socket reads internally.
            RespValue request = co_await parser.parse(socket);

            // 3. Execute using store
            // At this point, 'request' contains your parsed command 
            // (usually a RespType::Array of BulkStrings like ["SET", "key", "value"]).
            
            std::cout << "--- Received Command ---\n";
            print_resp(request);
            std::cout << "------------------------\n";
            
            // For now, let's mock a simple OK response so the loop completes.
            std::string mock_response = "+OK\r\n";

            // 4. Send the serialized response back to the client
            co_await asio::async_write(
                socket,
                asio::buffer(mock_response),
                asio::use_awaitable);
        }
    }
    catch (const std::exception& e)
    {
        // This catch block handles both malformed RESP errors thrown by your parser
        // and natural EOF (End of File) when the client disconnects.
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