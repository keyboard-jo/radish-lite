#include "server/Connection.hpp"
#include "protocol/RespParser.hpp"

#include <asio.hpp>
#include <iostream>

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
            // 
            // Example execution placeholder:
            // std::string response = execute_command(request, store);
            
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