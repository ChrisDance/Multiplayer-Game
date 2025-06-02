#pragma once
#include <cstring>
#include <cstdlib>
#include <mutex>
#include "raylib.h"
#include "raymath.h"
#include <queue>
#include "CircularBuffer.hpp"

#define INPUT_BUFFER_SIZE 10
#define MAX_PLAYER_COUNT 10
#define WORLD_WIDTH 400
#define WORLD_HEIGHT 300

enum class MSG
{
    CONNECT,
    DISCONNECT,
    PLAYER_UPDATE,
    WORLD_UPDATE,
    TIME_SYNC
};

struct Position
{
    Vector2 position;
    float time;
};

struct Player
{
    int id;
    CircularBuffer<Position> positions{10};
};

struct InputEntry
{
    uint64_t sequenceNum;
    uint8_t input[INPUT_BUFFER_SIZE];

    bool operator>(const InputEntry &other) const
    {
        return sequenceNum > other.sequenceNum;
    }
};

struct ClientInfo
{
    uint64_t lastCheckIn{0};
    int id;
    std::priority_queue<InputEntry, std::vector<InputEntry>, std::greater<InputEntry>> inputQueue;
    uint64_t lastProcessedSequence{0};
    Vector2 position;
};

struct PacketHeader
{
    MSG type;
};

struct TimeSyncPacket
{
    PacketHeader header{.type = MSG::TIME_SYNC};
    float serverTime;
    uint64_t startTimeNanos;
};

struct PlayerUpdatePacket
{
    PacketHeader header{.type = MSG::PLAYER_UPDATE};
    InputEntry entry;
};
struct WorldUpdatePacket
{
    PacketHeader header{.type = MSG::WORLD_UPDATE};
    float time;
    int playerCount;
    int playerIds[MAX_PLAYER_COUNT];
    Vector2 playerPositions[MAX_PLAYER_COUNT];
};
