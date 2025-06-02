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

    mRunning = true;
    if (!mSock.StartReceiveThread(std::chrono::milliseconds(10), callback))
    {
        std::cerr << "Failed to start receive thread\n";
        return;
    }
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

    case MSG::DISCONNECT:
    {
        mRunning = false;
        break;
    }

    default:
        break;
    }
}

void Client::Run()
{

    PacketHeader connectPacket = {.type = MSG::CONNECT};
    mSock.SendTo(&connectPacket, sizeof(PacketHeader), mServerAddr);
    constexpr std::chrono::milliseconds timeStep(1000);

    mRunning = true;

    int ticks = 0;
    while (mRunning)
    {
        std::this_thread::sleep_for(timeStep);

        std::string msg("hey there server!");
        mSock.SendTo(msg.c_str(), msg.size(), mServerAddr);

        if (ticks++ > 10)
        {
            mRunning = false;
        }
    }

    std::cout << "Shutting down\n";

    mSock.Close();
}
