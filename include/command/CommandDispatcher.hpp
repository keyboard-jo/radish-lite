#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include "protocol/RespValue.hpp"
#include "storage/KeyValueStore.hpp"

class CommandDispatcher {
public:
    explicit CommandDispatcher(KeyValueStore& store);

    std::string dispatch(const RespValue& request);
private:
    KeyValueStore& store_;

    using HandlerFunc = std::function<std::string(const std::vector<RespValue>&)>;

    std::unordered_map<std::string, HandlerFunc> registry_;

    void register_commands();
};