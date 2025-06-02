#pragma once
#include <cstring>
#include <cstdlib>
#include <mutex>
#include "raylib.h"
#include "raymath.h"

#define INPUT_BUFFER_SIZE 10
#define MAX_PLAYER_COUNT 10
#define WORLD_WIDTH 400
#define WORLD_HEIGHT 300

enum class MSG
{

    CONNECT,
    DISCONNECT,
    PLAYER_UPDATE,
    WORLD_UPDATE
};

struct Player
{
    Vector2 position;
};

struct ClientInfo
{
    uint64_t lastCheckIn{0};
    Player player;
};

/* Always have packet header as first member */
struct PacketHeader
{
    MSG type;
};

struct PlayerUpdatePacket
{
    PacketHeader header{.type = MSG::PLAYER_UPDATE};
    uint8_t input[INPUT_BUFFER_SIZE];
};

struct WorldUpdatePacket
{
    PacketHeader header{.type = MSG::WORLD_UPDATE};
    int playerCount;
    Player players[MAX_PLAYER_COUNT];
};