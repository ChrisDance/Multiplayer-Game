#pragma once
#include "UdpSocket.hpp"
#include "Shutdown.hpp"
#include "Shared.hpp"
#include "rlgl.h"
#include <map>

class Client
{

private:
    UdpSocket mSock;
    int mPort;
    sockaddr_in mServerAddr;
    bool mRunning{false};
    float mServerTime{0};
    std::chrono::high_resolution_clock::time_point mStartTime;
    uint64_t mSequenceNumber{0};

    std::map<int, Player> mPlayers;
    std::mutex mMutex;
    void ReceiveMessage(char *buffer, int bytesRead, sockaddr_in sender);
    void Render();
    uint8_t EncodeInput();
    Vector2 GetInterpolatedPosition(Player &player, float renderTime);

public:
    Client(int port, int serverPort);
    void Attach();
    void Run();
};