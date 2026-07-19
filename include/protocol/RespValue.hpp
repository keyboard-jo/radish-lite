#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class RespType
{
    SimpleString,
    Error,
    Integer,
    BulkString,
    Null,
    Array,

    // RESP3 (future)
    Boolean,
    Double,
    BigNumber,
    BulkError,
    VerbatimString,
    Map,
    Set,
    Push,
    Attribute
};

struct RespValue
{
    using Array = std::vector<RespValue>;

    RespType type = RespType::Null;

    // String-like payload
    std::string string;

    // Integer payload
    std::int64_t integer = 0;

    // Aggregate payload
    Array array;

    [[nodiscard]]
    bool is_array() const noexcept
    {
        return type == RespType::Array;
    }

    [[nodiscard]]
    bool is_string() const noexcept
    {
        return type == RespType::SimpleString ||
               type == RespType::BulkString ||
               type == RespType::Error;
    }

    [[nodiscard]]
    bool is_integer() const noexcept
    {
        return type == RespType::Integer;
    }

    [[nodiscard]]
    bool is_null() const noexcept
    {
        return type == RespType::Null;
    }

    [[nodiscard]]
    bool empty() const noexcept
    {
        switch (type)
        {
        case RespType::Array:
            return array.empty();

        case RespType::SimpleString:
        case RespType::BulkString:
        case RespType::Error:
            return string.empty();

        default:
            return false;
        }
    }

    [[nodiscard]]
    std::size_t size() const noexcept
    {
        return array.size();
    }

    [[nodiscard]]
    const RespValue& operator[](std::size_t index) const
    {
        return array[index];
    }

    [[nodiscard]]
    RespValue& operator[](std::size_t index)
    {
        return array[index];
    }
};