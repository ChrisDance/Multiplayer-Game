#pragma once
#include "UdpSocket.hpp"
#include "Shutdown.hpp"
#include "Shared.hpp"
#include "rlgl.h"

class Client
{

private:
    UdpSocket mSock;
    int mPort;
    sockaddr_in mServerAddr;
    bool mRunning{false};

    std::vector<Player> mPlayers;
    std::mutex mMutex;
    void ReceiveMessage(char *buffer, int bytesRead, sockaddr_in sender);
    void Render();
    uint8_t EncodeInput();

public:
    Client(int port, int serverPort);
    void Attach();
    void Run();
};