#include "command/CommandDispatcher.hpp"
#include <algorithm>
#include <cctype>
#include <charconv>

namespace {
    bool check_arity(size_t actual_size, size_t min_expected, size_t max_expected, std::string& error_response, std::string_view cmd_name) {
        if (actual_size < min_expected || actual_size > max_expected) {
            error_response = "-ERR wrong number of arguments for '" + std::string(cmd_name) + "' command\r\n";
            return false;
        }

        return true;
    }

    bool ensure_string_arg(const RespValue& val, std::string& error_response) {
        if (val.type != RespType::BulkString && val.type != RespType::SimpleString) {
            error_response = "-ERR operation against a key holding the wrong kind of value or invalid argument\r\n";
            return false;
        }

        return true;
    }
}

static std::string to_upper(std::string_view str) {
    std::string result(str);
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
        return std::toupper(c);
    });

    return result;
}

CommandDispatcher::CommandDispatcher(KeyValueStore& store) : store_(store) {
    register_commands();
}

void CommandDispatcher::register_commands() {
    registry_["PING"] = [this](const std::vector<RespValue>& args) {
        std::string err;
        if (!check_arity(args.size(), 0, 1, err, "ping")) return err;

        if (!args.empty()) {
            if (!ensure_string_arg(args[0], err)) return err;
            const std::string& msg = args[0].string;
            return "$" + std::to_string(msg.length()) + "\r\n" + msg + "\r\n";
        }

        return std::string("+PONG\r\n");
    };

    registry_["SET"] = [this](const std::vector<RespValue>& args) {
        std::string err;
        if (!check_arity(args.size(), 2, 4, err, "set")) return err;

        if (!ensure_string_arg(args[0], err) || !ensure_string_arg(args[1], err)) return err;

        const std::string& key = args[0].string;
        const std::string& val = args[1].string;
        
        std::optional<std::chrono::milliseconds> ttl = std::nullopt;

        if (args.size() > 2) {
            if (args.size() != 4) {
                return std::string("-ERR syntax error\r\n");
            }

            if (!ensure_string_arg(args[2], err) || !ensure_string_arg(args[3], err)) return err;

            std::string option = to_upper(args[2].string);
            if (option == "EX") {
                std::string_view ttl_str = args[3].string;
                int64_t parsed_ttl = 0;

                auto [ptr, ec] = std::from_chars(ttl_str.data(), ttl_str.data() + ttl_str.size(), parsed_ttl);

                if (ec == std::errc::result_out_of_range || ec != std::errc() || ptr != ttl_str.data() + ttl_str.size()) {
                    return std::string("-ERR value is not an integer or out of range\r\n");
                }

                if (parsed_ttl <= 0) {
                    return std::string("-ERR invalid expire time in 'set' command\r\n");
                }

                // Convert parsed seconds to milliseconds
                ttl = std::chrono::seconds(parsed_ttl);
            } else {
                return std::string("-ERR syntax error\r\n");
            }
        }

        store_.set(key, val, ttl);
        return std::string("+OK\r\n");
    };

    registry_["GET"] = [this](const std::vector<RespValue>& args) {
        std::string err;
        if (!check_arity(args.size(), 1, 1, err, "get")) return err;

        if (!ensure_string_arg(args[0], err)) return err;

        const std::string& key = args[0].string;

        auto val = store_.get(key);
        if (val.has_value()) {
            return "$" + std::to_string(val->length()) + "\r\n" + *val + "\r\n";
        } else {
            return std::string("$-1\r\n");
        }
    };
}


std::string CommandDispatcher::dispatch(const RespValue& request) {
    if (request.type != RespType::Array || request.empty()) {
        return "-ERR invalid command format\r\n";
    }

    const auto& cmd_element = request.array[0];
    if (cmd_element.type != RespType::BulkString && cmd_element.type != RespType::SimpleString) {
        return "-ERR invalid command name\r\n";
    }

    std::string cmd = to_upper(cmd_element.string);

    auto it = registry_.find(cmd);
    if (it != registry_.end()) {
        std::vector<RespValue> args(request.array.begin() + 1, request.array.end());
        return it->second(args);
    }

    return "-ERR unknown command '" + cmd + "'\r\n";
}
