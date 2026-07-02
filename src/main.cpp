#include <iostream>
#include <sstream>
#include "KeyValueStore.hpp"

int main(){
    KeyValueStore kv;
    std::string line;

    std::cout << "Radish-Lite Engine Initialized.\n";
    std::cout << "Commands: SET <key> <val> <ttl_sec> | GET <key> | PEEK <key> | EXIT\n\n";

    while (true) {
        std::cout << "radish> ";
        if (!std::getline(std::cin, line)) break;

        std::stringstream ss(line);
        std::string command;

        ss >> command;

        if (command =="EXIT" || command == "exit") {
            break;
        }
        else if (command == "SET" || command == "set") {
            std::string key, val;
            int ttl;

            if (ss >> key >> val >> ttl) {
                kv.set(key, val, ttl);
                std::cout << "OK\n";
            } else {
                std::cout << "ERR: Usage SET <key> <val> <ttl_seconds>\n";
            }
        }
        else if (command == "GET" || command == "get") {
            std::string key;

            if (ss >> key) {
                auto val = kv.get(key);

                if (val.has_value()) {
                    std::cout << "\"" << val.value() << "\"\n";
                } else {
                    std::cout << "(nil)\n";
                }
            } else {
                std::cout << "ERR: Usage: GET <key>\n";
            }
        }
        else if (command == "PEEK" || command == "peek") {
            std::string key;
            if (ss >> key) {
                auto val = kv.peek(key);

                if (val.has_value()) {
                    std::cout << "\"" << val.value() << "\"\n";
                } else {
                    std::cout << "(nil)\n";
                }
            } else {
                std::cout << "ERR: Usage: PEEK <key>";
            }
        } 
        else if (!command.empty()) {
            std::cout << "ERR: Unknown command '" << command << "'\n";
        }
    };

    return 0;
}
