#include "server/Connection.hpp"
#include "protocol/RespParser.hpp"

#include <asio.hpp>
#include <iostream>

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

asio::awaitable<void> handle_client(
    asio::ip::tcp::socket socket,
    KeyValueStore& store)
{
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