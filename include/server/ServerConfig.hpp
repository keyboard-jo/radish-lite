#pragma once

#include <string>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <asio.hpp>


struct ServerConfig {
    std::string host = "0.0.0.0";
    uint16_t    port = 6379;
    std::size_t max_buffer_size = 32 * 1024 * 1024;

    operator asio::ip::tcp::endpoint() const {
        using namespace asio::ip;
        return {make_address(host.empty() ? "0.0.0.0" : host), port};
    }

    static ServerConfig load_from_file(const std::string& filepath) {
        ServerConfig config;
        std::ifstream file(filepath);

        if (!file.is_open()) {
            return config;
        }

        std::string line;

        while (std::getline(file, line)) {
            line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));

            if (line.empty() || line[0] == '#') {
                continue;
            }

            std::istringstream iss(line);
            std::string key;
            std::string value;

            if (iss >> key >> value) {
                if (key == "host") {
                    config.host = value;
                } else if (key == "port") {
                    config.port = static_cast<uint16_t>(std::stoi(value));
                } else if (key == "max_buffer_size") {
                    config.max_buffer_size = std::stoull(value);
                }
            }
        }

        return config;
    }
};