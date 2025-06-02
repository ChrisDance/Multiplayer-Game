#pragma once
#include "UdpSocket.hpp"
#include <set>

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
    std::set<sockaddr_in, SockAddrCompare> mClients;

    void ReceiveMessage(char *buffer, int bytesRead, sockaddr_in sender);

public:
    Server(int port);

    void Attach();

    void Run();
};
