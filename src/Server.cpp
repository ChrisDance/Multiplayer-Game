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
    constexpr std::chrono::
        milliseconds timeStep(mServerStepMs);
    mRunning = true;

    while (mRunning && !Shutdown::should_shutdown())
    {
        Step();
        std::this_thread::sleep_for(timeStep);
    }

    PacketHeader packet = {
        .type = MSG::DISCONNECT};
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
        mClients[sender] = ClientInfo{.lastCheckIn = 0};
        return;
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
            for (int i = 0; i < INPUT_BUFFER_SIZE; i++)
            {
                ApplyInput(&mClients[sender].player.position, data->input[i]);
            }

            break;
        }
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

            std::cout << it->second.lastCheckIn << '\n';
            ++it;
        }
    }

    WorldUpdatePacket packet;

    int i = 0;
    for (auto &[address, client] : mClients)
    {
        packet.players[i++] = client.player;
    }

    packet.playerCount = mClients.size();

    Broadcast(&packet, sizeof(WorldUpdatePacket));
}

void Server::Broadcast(void *data, int size)
{
    for (auto &[address, client] : mClients)
    {
        mSock.SendTo(data, size, address);
    }
}

void Server::ApplyInput(Vector2 *position, uint8_t input)
{
    const float MOVE_SPEED = (10.0f);

    if (input & (1 << 0))
        position->y -= MOVE_SPEED;
    if (input & (1 << 1))
        position->y += MOVE_SPEED;
    if (input & (1 << 2))
        position->x += MOVE_SPEED;
    if (input & (1 << 3))
        position->x -= MOVE_SPEED;
}
