#pragma once

#include <asio.hpp>
#include "KeyValueStore.hpp"

asio::awaitable<void> handle_client(
    asio::ip::tcp::socket socket,
    KeyValueStore& store
);