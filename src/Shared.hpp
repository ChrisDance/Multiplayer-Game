#pragma once
#include <cstring>
#include <cstdlib>

enum class MSG
{
    CONNECT,
    DISCONNECT
};

struct ClientInfo
{
    uint64_t lastCheckIn{0};
};

struct PacketHeader
{
    MSG type;
};
