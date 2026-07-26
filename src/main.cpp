#include <asio.hpp>
#include <iostream>
#include <sstream>

#include "storage/KeyValueStore.hpp"
#include "server/Server.hpp"
#include "server/ServerConfig.hpp"



int main(int argc, char* argv[]){
    std::string config_path = (argc > 1) ? argv[1] : "radish.conf";
    ServerConfig config = ServerConfig::load_from_file(config_path);

    asio::io_context ioContext;
    KeyValueStore kv(ioContext);

    Server server(ioContext, config, kv);

    ioContext.run();
    return 0;
}
