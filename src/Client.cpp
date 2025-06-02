#include "Client.hpp"

Client::Client(int port, int serverPort) : mPort(port)
{
    mServerAddr = UdpSocket::CreateAddress("127.0.0.1", serverPort);
}

void Client::Attach()
{

    if (!mSock.Create("127.0.0.1", mPort))
    {
        std::cerr << "Couldn't create socket\n";
        return;
    }

    std::function<void(char *buffer, int bytesRead, sockaddr_in sender)> callback =
        [this](char *buffer, int bytesRead, sockaddr_in sender)
    {
        this->ReceiveMessage(buffer, bytesRead, sender);
    };

    if (!mSock.StartReceiveThread(std::chrono::milliseconds(10), callback))
    {
        std::cerr << "Failed to start receive thread\n";
        return;
    }

    mRunning = true;
    PacketHeader connectPacket = {.type = MSG::CONNECT};
    mSock.SendTo(&connectPacket, sizeof(PacketHeader), mServerAddr);
}

void Client::ReceiveMessage(char *buffer, int bytesRead, sockaddr_in sender)
{
    if (sender.sin_addr.s_addr != mServerAddr.sin_addr.s_addr || sender.sin_port != mServerAddr.sin_port)
    {
        return;
    }

    auto packet = reinterpret_cast<PacketHeader *>(buffer);

    switch (packet->type)
    {
    case MSG::TIME_SYNC:
    {
        auto data = reinterpret_cast<TimeSyncPacket *>(buffer);

        mStartTime = std::chrono::high_resolution_clock::time_point(
            std::chrono::nanoseconds(data->startTimeNanos));

        std::cout << "Time sync - Server time: " << data->serverTime << "ms\n";
        break;
    }
    case MSG::DISCONNECT:
    {
        mRunning = false;
        break;
    }

    case MSG::WORLD_UPDATE:
    {
        std::lock_guard<std::mutex> lock(mMutex);
        auto data = reinterpret_cast<WorldUpdatePacket *>(buffer);
        for (int i = 0; i < data->playerCount; i++)
        {
            mPlayers[data->playerIds[i]].positions.push({data->playerPositions[i], data->time});
            mPlayers[data->playerIds[i]].radius = data->playerRadius[i];
        }

        break;
    }
    case MSG::DOT_UPDATE:
    {
        auto data = reinterpret_cast<DotUpdatePacket *>(buffer);
        {
            for (int i = 0; i < DOT_COUNT; i++)
            {
                mDots[i] = data->positions[i];
            }
        }
        break;
    }

    default:
        break;
    }
}

void Client::Run()
{
    if (!mRunning)
    {
        return;
    }

    PacketHeader connectPacket = {.type = MSG::CONNECT};
    mSock.SendTo(&connectPacket, sizeof(PacketHeader), mServerAddr);

    InitWindow(WORLD_WIDTH, WORLD_HEIGHT, "Multiplayer");
    SetTargetFPS(100);

    PlayerUpdatePacket packet;

    while (mRunning && !WindowShouldClose())
    {

        auto currentTime = std::chrono::high_resolution_clock::now();
        mServerTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                          currentTime - mStartTime)
                          .count();

        Render();

        uint8_t input = EncodeInput();
        packet.entry.input[mSequenceNumber % INPUT_BUFFER_SIZE] = input;

        if (mSequenceNumber % INPUT_BUFFER_SIZE == 0)
        {
            packet.entry.sequenceNum = mSequenceNumber;
            mSock.SendTo(&packet, sizeof(PlayerUpdatePacket), mServerAddr);
        }

        mSequenceNumber++;
    }

    PacketHeader disconnect = {.type = MSG::DISCONNECT};
    mSock.SendTo(&disconnect, sizeof(PacketHeader), mServerAddr);
    mSock.Close();
}

void Client::Render()
{

    Camera2D camera = {};
    camera.offset = (Vector2){WORLD_WIDTH / 2.0f, WORLD_HEIGHT / 2.0f};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode2D(camera);

    /*draw grid*/
    rlPushMatrix();
    rlTranslatef(0, 25 * 50, 0);
    rlRotatef(90, 1, 0, 0);
    DrawGrid(100, 50);
    rlPopMatrix();

    for (int i = 0; i < DOT_COUNT; i++)
    {
        DrawCircle(mDots[i].x, mDots[i].y, DOT_RADIUS, GREEN);
    }
    float renderTime = mServerTime - 200;

    mMutex.lock();
    for (auto &[id, player] : mPlayers)
    {
        Vector2 position = GetInterpolatedPosition(player, renderTime);
        DrawCircle(position.x, position.y, player.radius, mPort == id ? RED : BLUE);
    }

    mMutex.unlock();
    EndMode2D();
    EndDrawing();
}

Vector2 Client::GetInterpolatedPosition(Player &player, float renderTime)
{
    // Find the two snapshots that bracket the target time
    Position *before = nullptr;
    Position *after = nullptr;

    int size = player.positions.size();
    auto &positionHistory = player.positions;

    for (int i = 0; i < size - 1; i++)
    {
        if (positionHistory.at(i).time <= renderTime &&
            positionHistory.at(i + 1).time >= renderTime)
        {
            before = &positionHistory.at(i);
            after = &positionHistory.at(i + 1);
            break;
        }
    }

    if (!before || !after)
    {
        if (!positionHistory.size())
        {
            return {0, 0};
        }
        /* If for whatever reason, our render time is outside of the buffered positions*/
        std::cout << "time: " << renderTime << ", latest: " << positionHistory.back().time << ", last: " << positionHistory.front().time << '\n';
        return positionHistory.back().position;
    }

    float t = static_cast<float>(renderTime - before->time) /
              static_cast<float>(after->time - before->time);

    return Vector2{
        Lerp(before->position.x, after->position.x, t),
        Lerp(before->position.y, after->position.y, t)};
}

uint8_t Client::EncodeInput()
{
    uint8_t input = 0;
    if (IsKeyDown(KEY_UP))
        input |= (1 << 0);
    if (IsKeyDown(KEY_DOWN))
        input |= (1 << 1);
    if (IsKeyDown(KEY_RIGHT))
        input |= (1 << 2);
    if (IsKeyDown(KEY_LEFT))
        input |= (1 << 3);
    if (IsKeyDown(KEY_SPACE))
        input |= (1 << 4);

    return input;
}