#pragma once
#include "UdpSocket.hpp"
#include "Shared.hpp"
#include "Shutdown.hpp"
#include <mutex>
#include <map>

class Server
{
    struct SockAddrCompare
    {
        bool operator()(const sockaddr_in &a, const sockaddr_in &b) const
        {

            if (a.sin_addr.s_addr != b.sin_addr.s_addr)
            {
                return a.sin_addr.s_addr < b.sin_addr.s_addr;
            }

            return a.sin_port < b.sin_port;
        }
    };

private:
    UdpSocket mSock;
    int mPort;
    bool mRunning{false};
    static const int mServerStepMs = 100;
    std::map<sockaddr_in, ClientInfo, SockAddrCompare> mClients;
    std::mutex mMutex;
    std::chrono::high_resolution_clock::time_point mStartTime;
    float mTime{0.0f};
    Vector2 mDots[DOT_COUNT];

    void ReceiveMessage(char *buffer, int bytesRead, sockaddr_in sender);
    void Step();
    void Broadcast(void *data, int size);
    void CreateDots();
    Vector2 GetRandomPosition();
    void CheckPlayerCollisions();
    void CheckDotCollisions();

public:
    Server(int port);

    void Attach();

    void Run();
};
