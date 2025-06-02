#include "Server.hpp"

Server::Server(int port) : mPort(port) {};

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
    using namespace std::chrono;
    constexpr milliseconds timeStep(1000);

    int ticks = 0;
    std::string msg("Hello there client!");

    while (mRunning)
    {
        std::this_thread::sleep_for(timeStep);

        for (auto &client : mClients)
        {
            mSock.SendTo(msg.c_str(), msg.size(), client);
        }

        if (ticks++ > 10)
        {
            mRunning = false;
        }
    }

    mSock.Close();
}

void Server::ReceiveMessage(char *buffer, int bytesRead, sockaddr_in sender)
{

    mClients.insert(sender);
    std::cout << "message from client " << sender.sin_port << ": " << buffer << '\n';
}