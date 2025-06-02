#pragma once
#include "UdpSocket.hpp"

class Client
{

private:
    UdpSocket mSock;
    int mPort;
    sockaddr_in mServerAddr;
    bool mRunning{false};
    void ReceiveMessage(char *buffer, int bytesRead, sockaddr_in sender);

public:
    Client(int port, int serverPort);
    void Attach();
    void Run();
};