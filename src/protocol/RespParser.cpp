#include "protocol/RespParser.hpp"
#include <stdexcept>

RespParser::ParseResult RespParser::parse(std::string_view data)
{
    RespValue value;
    std::size_t offset = 0;

    if (parse_value(data, offset, value))
    {
        return { std::move(value), offset };
    }

    return { std::nullopt, 0 };
}

bool RespParser::parse_value(std::string_view data, std::size_t& offset, RespValue& out)
{
    if (offset >= data.size()) return false;

    char type = data[offset];
    switch (type)
    {
    case '+': return parse_simple_string(data, offset, out);
    case '-': return parse_error(data, offset, out);
    case ':': return parse_integer_value(data, offset, out);
    case '$': return parse_bulk_string(data, offset, out);
    case '*': return parse_array(data, offset, out);
    default: throw std::runtime_error("Unknown RESP type.");
    }
}

bool RespParser::parse_simple_string(std::string_view data, std::size_t& offset, RespValue& out)
{
    auto cr = data.find("\r\n", offset);
    if (cr == std::string_view::npos) return false;

    out.type = RespType::SimpleString;
    out.string = std::string(data.substr(offset + 1, cr - (offset + 1)));
    
    offset = cr + 2;
    return true;
}

bool RespParser::parse_error(std::string_view data, std::size_t& offset, RespValue& out)
{
    auto cr = data.find("\r\n", offset);
    if (cr == std::string_view::npos) return false;

    out.type = RespType::Error;
    out.string = std::string(data.substr(offset + 1, cr - (offset + 1)));
    
    offset = cr + 2;
    return true;
}

bool RespParser::parse_integer_value(std::string_view data, std::size_t& offset, RespValue& out)
{
    auto cr = data.find("\r\n", offset);
    if (cr == std::string_view::npos) return false;

    out.type = RespType::Integer;
    out.integer = parse_integer(data.substr(offset + 1, cr - (offset + 1)));

    offset = cr + 2;
    return true;
}

bool RespParser::parse_bulk_string(std::string_view data, std::size_t& offset, RespValue& out)
{
    auto cr = data.find("\r\n", offset);
    if (cr == std::string_view::npos) return false;

    std::size_t length = static_cast<std::size_t>(
        parse_integer(data.substr(offset + 1, cr - (offset + 1))));

    // Calculate total bytes required including string length and the trailing \r\n
    std::size_t data_start = cr + 2;
    std::size_t total_required = data_start + length + 2;

    if (data.size() < total_required) return false;

    out.type = RespType::BulkString;
    out.string = std::string(data.substr(data_start, length));
    
    offset = total_required;
    return true;
}

bool RespParser::parse_array(std::string_view data, std::size_t& offset, RespValue& out)
{
    auto cr = data.find("\r\n", offset);
    if (cr == std::string_view::npos) return false;

    std::size_t count = static_cast<std::size_t>(
        parse_integer(data.substr(offset + 1, cr - (offset + 1))));

    std::size_t current_offset = cr + 2;

    out.type = RespType::Array;
    out.array.clear();
    out.array.reserve(count);

    for (std::size_t i = 0; i < count; ++i)
    {
        RespValue elem;
        if (!parse_value(data, current_offset, elem))
        {
            return false; // Not enough data yet to complete this array
        }
        out.array.push_back(std::move(elem));
    }

    offset = current_offset;
    return true;
}

std::int64_t RespParser::parse_integer(std::string_view text)
{
    bool negative = false;
    std::size_t i = 0;

    if (!text.empty() && text.front() == '-')
    {
        negative = true;
        i = 1;
    }

    std::int64_t value = 0;

    for (; i < text.size(); ++i)
    {
        char c = text[i];
        if (c < '0' || c > '9')
            throw std::runtime_error("Invalid integer.");

        value = value * 10 + (c - '0');
    }

    return negative ? -value : value;
}