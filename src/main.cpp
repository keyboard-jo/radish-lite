#include <asio.hpp>
#include <iostream>
#include <sstream>

#include "KeyValueStore.hpp"
#include "Server.hpp"



int main(){
    asio::io_context ioContext;

    KeyValueStore kv;

    Server server(ioContext, 6379, kv);

    ioContext.run();

    return 0;
}
