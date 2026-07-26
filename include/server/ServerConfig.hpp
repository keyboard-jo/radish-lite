#pragma once

#include <string>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <chrono>
#include <asio.hpp>


struct ServerConfig {
    std::string             host = "0.0.0.0";
    uint16_t                port = 6379;
    std::size_t             default_buffer_size = 8192;
    std::size_t             max_buffer_size = 32 * 1024 * 1024;
    std::chrono::seconds    idle_timeout{30};
    std::chrono::seconds    io_timeout{5};

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
        size_t line_number = 0;

        while (std::getline(file, line)) {
            line_number++;
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
                try {
                    if (key == "host") {
                        config.host = value;
                    } else if (key == "port") {
                        unsigned long parsed_val = std::stoul(value);
                        if (parsed_val > 65535) {
                            throw std::out_of_range("Port out of valid range (0-65535)");
                        }
                        config.port = static_cast<uint16_t>(parsed_val);
                    } else if (key == "max_buffer_size") {
                        config.max_buffer_size = std::stoull(value);
                    } else if (key == "default_buffer_size") {
                        config.default_buffer_size = std::stoull(value);
                    } else if (key == "idle_timeout_duration") {
                        config.idle_timeout = std::chrono::seconds(std::stoul(value));
                    } else if (key == "io_timeout") {
                        config.io_timeout = std::chrono::seconds(std::stoul(value));
                    }
                } catch (const std::exception& e) {
                    throw std::runtime_error("Config parsing error at line " + std::to_string(line_number) + " for key '" + key + "': " + e.what());
                }
            }
        }

        return config;
    }
};