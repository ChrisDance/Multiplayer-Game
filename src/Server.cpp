#include "Server.hpp"

Server::Server(int port) : mPort(port)
{
    Shutdown::setup();
};

void Server::Attach()
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

    mRunning = true;
    if (!mSock.StartReceiveThread(std::chrono::milliseconds(10), callback))
    {
        std::cerr << "Failed to start receive thread\n";
        return;
    }
}

void Server::Run()
{

    CreateDots();

    using namespace std::chrono;
    constexpr milliseconds timeStep(mServerStepMs);

    mStartTime = std::chrono::high_resolution_clock::now();

    while (mRunning && !Shutdown::should_shutdown())
    {
        auto currentTime = high_resolution_clock::now();
        mTime = duration_cast<milliseconds>(currentTime - mStartTime).count();
        Step();

        auto workTime = high_resolution_clock::now() - currentTime;
        auto sleepTime = timeStep - workTime;

        if (sleepTime > nanoseconds(0))
        {
            std::this_thread::sleep_for(sleepTime);
        }
    }

    PacketHeader packet = {.type = MSG::DISCONNECT};
    for (auto &[address, info] : mClients)
    {
        mSock.SendTo(&packet, sizeof(PacketHeader), address);
    }

    mSock.Close();
    std::cout << "Shutting down\n";
}

void Server::ReceiveMessage(char *buffer, int bytesRead, sockaddr_in sender)
{

    std::lock_guard<std::mutex> lock(mMutex);
    auto packet = reinterpret_cast<PacketHeader *>(buffer);

    if (mClients.find(sender) == mClients.end() && packet->type == MSG::CONNECT)
    {
        mClients[sender] = ClientInfo{.lastCheckIn = 0, .id = ntohs(sender.sin_port)};
        PacketHeader p1;
        p1.type = MSG::CONNECT;
        mSock.SendTo(&p1, sizeof(PacketHeader), sender);

        DotUpdatePacket p2;
        memcpy(&p2.positions, mDots, sizeof(Vector2) * DOT_COUNT);
        mSock.SendTo(&p2, sizeof(DotUpdatePacket), sender);

        TimeSyncPacket p3;
        p3.startTimeNanos = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                mStartTime.time_since_epoch())
                                .count();
        mSock.SendTo(&p3, sizeof(TimeSyncPacket), sender);
    }
    else
    {
        mClients[sender].lastCheckIn = 0;

        switch (packet->type)
        {
        case MSG::DISCONNECT:
        {
            mClients.erase(sender);
            std::cout << "Client disconnected\n";
            break;
        }
        case MSG::PLAYER_UPDATE:
        {
            auto data = reinterpret_cast<PlayerUpdatePacket *>(buffer);
            mClients[sender].inputQueue.push(data->entry);
            break;
        }
        default:
            break;
        }
    }
}

void Server::Step()
{

    const int heartBeatCutoff = 10;
    std::lock_guard<std::mutex> lock(mMutex);
    for (auto it = mClients.begin(); it != mClients.end();)
    {
        if (it->second.lastCheckIn++ > heartBeatCutoff)
        {
            PacketHeader disconnectPacket = {.type = MSG::DISCONNECT};
            mSock.SendTo(&disconnectPacket, sizeof(PacketHeader), it->first);
            it = mClients.erase(it);
            std::cout << "Client disconnected\n";
        }
        else
        {
            ++it;
        }
    }

    WorldUpdatePacket packet;
    packet.time = mTime;

    int i = 0;
    for (auto &[address, client] : mClients)
    {
        int inputsProcessed = 0;
        const int maxInputsPerFrame = 10;
        while (!client.inputQueue.empty() && inputsProcessed < maxInputsPerFrame)
        {
            const InputEntry &entry = client.inputQueue.top();

            if (entry.sequenceNum <= client.lastProcessedSequence)
            {
                client.inputQueue.pop();
                continue;
            }

            for (auto &input : entry.input)
            {
                ApplyInput(&client.position, input, client.radius);
            }

            client.lastProcessedSequence = entry.sequenceNum;

            client.inputQueue.pop();
            inputsProcessed++;
        }

        const size_t maxQueueSize = 100;

        while (client.inputQueue.size() > maxQueueSize)
        {
            client.inputQueue.pop();
        }

        packet.playerIds[i] = client.id;
        packet.playerRadius[i] = client.radius;
        packet.playerPositions[i++] = client.position;
    }

    packet.playerCount = mClients.size();

    CheckPlayerCollisions();
    CheckDotCollisions();

    Broadcast(&packet, sizeof(WorldUpdatePacket));
}

void Server::Broadcast(void *data, int size)
{
    for (auto &[address, client] : mClients)
    {
        mSock.SendTo(data, size, address);
    }
}

void Server::ApplyInput(Vector2 *position, uint8_t input, uint32_t radius)
{
    const float MOVE_SPEED = (10.0f) / std::max(radius, (uint32_t)(10));

    if (input & (1 << 0))
        position->y -= MOVE_SPEED;
    if (input & (1 << 1))
        position->y += MOVE_SPEED;
    if (input & (1 << 2))
        position->x += MOVE_SPEED;
    if (input & (1 << 3))
        position->x -= MOVE_SPEED;
}
void Server::CheckPlayerCollisions()
{
    for (auto &[id1, player1] : mClients)
    {
        for (auto &[id2, player2] : mClients)
        {
            if (player1.id == player2.id)
            {
                continue;
            }

            if (Vector2Distance(player1.position, player2.position) < player1.radius)
            {
                if (player1.radius > player2.radius)
                {
                    player1.radius += player2.radius;
                    player2.radius = 10;
                    player2.position = GetRandomPosition();
                }
            }
        }
    }
}

void Server::CheckDotCollisions()
{
    bool broadCast = false;
    for (auto &[id, player] : mClients)
    {
    next:
        for (auto &dot : mDots)
        {

            if (Vector2Distance(player.position, dot) <= player.radius)
            {

                player.radius += 1;
                dot = GetRandomPosition();
                broadCast = true;
                goto next;
            }
        }
    }

    if (broadCast)
    {
        DotUpdatePacket packet;
        memcpy(&packet.positions, &mDots, sizeof(Vector2) * DOT_COUNT);
        Broadcast(&packet, sizeof(DotUpdatePacket));
    }
}

void Server::CreateDots()
{
    for (int i = 0; i < DOT_COUNT; i++)
    {
        mDots[i] = GetRandomPosition();
    }
}

Vector2 Server::GetRandomPosition()
{
    return {(float)GetRandomValue(-(WORLD_WIDTH / 2), (WORLD_HEIGHT / 2)),
            (float)GetRandomValue(-(WORLD_WIDTH / 2), (WORLD_HEIGHT / 2))};
}
