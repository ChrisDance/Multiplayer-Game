#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>
#include <functional>
#include <thread>
#include <atomic>
#include <chrono>

class UdpSocket
{

private:
    int mSockFd{-1};
    const unsigned int mMaxPacketSize{1024};
    std::atomic<bool> mReceiving{false};
    std::thread mReceiveThread;

    std::function<void(char *buffer, int bytesRead, sockaddr_in sender)> mCallback = nullptr;

public:
    sockaddr_in mBoundAddress;

    UdpSocket() {}
    ~UdpSocket()
    {
        Close();
    }

    UdpSocket(const UdpSocket &) = delete;
    UdpSocket &operator=(const UdpSocket &) = delete;

    bool Create(const char *ip, unsigned short port)
    {

        if (mSockFd >= 0)
        {
            close(mSockFd);
        }

        mSockFd = socket(AF_INET, SOCK_DGRAM, 0);
        if (mSockFd < 0)
        {
            std::cerr << "Failed to create socket: " << strerror(errno) << '\n';
            return false;
        }

        mBoundAddress = UdpSocket::CreateAddress(ip, port);

        int result = bind(mSockFd, (struct sockaddr *)&mBoundAddress, sizeof(mBoundAddress));
        if (result < 0)
        {
            std::cerr << "Failed to bind socket: " << strerror(errno) << '\n';
            return false;
        }

        return true;
    }

    static sockaddr_in CreateAddress(const char *ip, unsigned short port)
    {
        sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);

        if (ip == nullptr || strcmp(ip, "0.0.0.0") == 0)
        {
            addr.sin_addr.s_addr = htonl(INADDR_ANY);
        }
        else
        {
            addr.sin_addr.s_addr = inet_addr(ip);
            if (addr.sin_addr.s_addr == INADDR_NONE)
            {
                std::cerr << "Invalid IP address: " << ip << '\n';
            }
        }
        return addr;
    }

    void Receive()
    {
        if (mCallback == nullptr)
        {
            std::cerr << "No callback set, call SetCallback() first\n";
            return;
        }

        char buffer[mMaxPacketSize];
        sockaddr_in sender;

        while (mReceiving)
        {
            socklen_t senderSize = sizeof(sender);

            int bytesRead = recvfrom(mSockFd, buffer, mMaxPacketSize, 0, (struct sockaddr *)&sender, &senderSize);

            if (bytesRead > 0)
            {

                buffer[bytesRead < mMaxPacketSize ? bytesRead : mMaxPacketSize - 1] = '\0';

                mCallback(buffer, bytesRead, sender);
            }
            else if (bytesRead < 0)
            {

                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    continue;
                }
                else
                {

                    std::cerr << "Error in receive thread: " << strerror(errno) << '\n';
                    break;
                }
            }
        }
    }

    bool StartReceiveThread(std::chrono::milliseconds time, std::function<void(char *buffer, int bytesRead, sockaddr_in sender)> callback)
    {
        if (mSockFd < 0)
        {
            std::cerr << "Socket not created, call 'Create()' first\n";
            return false;
        }
        if (mReceiving)
        {
            std::cerr << "Receive thread already running\n";
            return false;
        }

        mCallback = callback;

        struct timeval timeout;
        timeout.tv_sec = time.count() / 1000;
        timeout.tv_usec = (time.count() % 1000) * 1000;

        if (setsockopt(mSockFd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
        {
            std::cerr << "Failed to set socket timeout: " << strerror(errno) << '\n';
            return false;
        }

        mReceiving = true;

        mReceiveThread = std::thread(&UdpSocket::Receive, this);

        return true;
    }

    int SendTo(const void *data, int size, const sockaddr_in &dest)
    {
        if (mSockFd < 0)
        {
            std::cerr << "Socket not created, call 'Create()' first\n";
            return -1;
        }

        int bytesSent = sendto(mSockFd, data, size, 0, (struct sockaddr *)&dest, sizeof(dest));
        if (bytesSent < 0)
        {
            std::cerr << "Failed to send data: " << strerror(errno) << '\n';
        }
        return bytesSent;
    }

    void Close()
    {
        mReceiving = false;

        if (mSockFd >= 0)
        {
            shutdown(mSockFd, SHUT_RDWR);
        }

        if (mReceiveThread.joinable())
        {
            mReceiveThread.join();
        }

        if (mSockFd >= 0)
        {
            close(mSockFd);
            mSockFd = -1;
        }
    }
};