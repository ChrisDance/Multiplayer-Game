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
        milliseconds timeStep(1000);
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
            break;
        }
        }

        std::cout << "message from client " << sender.sin_port << ": " << buffer << '\n';
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
}
