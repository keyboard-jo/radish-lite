#pragma once

#include <asio.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "protocol/RespValue.hpp"

class RespParser
{
public:
    struct ParseResult
    {
        std::optional<RespValue> value;
        std::size_t consumed = 0;
    };

    [[nodiscard]]
    static ParseResult parse(std::string_view data);

private:
    static bool parse_value(std::string_view data, std::size_t& offset, RespValue& out);
    static bool parse_simple_string(std::string_view data, std::size_t& offset, RespValue& out);
    static bool parse_error(std::string_view data, std::size_t& offset, RespValue& out);
    static bool parse_integer_value(std::string_view data, std::size_t& offset, RespValue& out);
    static bool parse_bulk_string(std::string_view data, std::size_t& offset, RespValue& out);
    static bool parse_array(std::string_view data, std::size_t& offset, RespValue& out);

    [[nodiscard]]
    static std::int64_t parse_integer(std::string_view text);
};