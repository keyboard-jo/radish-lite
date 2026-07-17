#pragma once

#include <string>
#include <vector>

struct RespValue
{
    using Array = std::vector<std::string>;

    Array elements;

    [[nodiscard]]
    bool empty() const noexcept
    {
        return elements.empty();
    }

    [[nodiscard]]
    std::size_t size() const noexcept
    {
        return elements.size();
    }

    [[nodiscard]]
    const std::string& operator[](std::size_t index) const
    {
        return elements[index];
    }

    [[nodiscard]]
    std::string& operator[](std::size_t index)
    {
        return elements[index];
    }
};