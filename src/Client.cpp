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

    if (!mSock.StartReceiveThread(std::chrono::milliseconds(10), callback))
    {
        std::cerr << "Failed to start receive thread\n";
        return;
    }

    mRunning = true;
    PacketHeader connectPacket = {.type = MSG::CONNECT};
    mSock.SendTo(&connectPacket, sizeof(PacketHeader), mServerAddr);
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

    case MSG::WORLD_UPDATE:
    {
        std::lock_guard<std::mutex> lock(mMutex);
        auto data = reinterpret_cast<WorldUpdatePacket *>(buffer);
        mPlayers.clear();
        for (int i = 0; i < data->playerCount; i++)
        {
            mPlayers.push_back(data->players[i]);
        }

        break;
    }

    default:
        break;
    }
}

void Client::Run()
{
    if (!mRunning)
    {
        return;
    }
    InitWindow(WORLD_WIDTH, WORLD_HEIGHT, "Multiplayer Game");
    SetTargetFPS(50);

    uint64_t sequenceNumber = 0;

    PlayerUpdatePacket packet;

    while (mRunning && !WindowShouldClose())
    {
        Render();

        uint8_t input = EncodeInput();
        packet.input[sequenceNumber % INPUT_BUFFER_SIZE] = input;

        if (sequenceNumber % INPUT_BUFFER_SIZE == 0)
        {
            mSock.SendTo(&packet, sizeof(PlayerUpdatePacket), mServerAddr);
        }

        sequenceNumber++;
    }

    PacketHeader disconnect = {
        .type = MSG::DISCONNECT};
    mSock.SendTo(&disconnect, sizeof(PacketHeader), mServerAddr);
    mSock.Close();
}

void Client::Render()
{

    Camera2D camera = {};
    camera.offset = (Vector2){WORLD_WIDTH / 2.0f, WORLD_HEIGHT / 2.0f};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode2D(camera);

    /*draw grid*/
    rlPushMatrix();
    rlTranslatef(0, 25 * 50, 0);
    rlRotatef(90, 1, 0, 0);
    DrawGrid(100, 50);
    rlPopMatrix();

    mMutex.lock();
    for (auto &player : mPlayers)
    {
        DrawCircle(player.position.x, player.position.y, 10, RED);
    }

    mMutex.unlock();
    EndMode2D();
    EndDrawing();
}

uint8_t Client::EncodeInput()
{
    uint8_t input = 0;
    if (IsKeyDown(KEY_UP))
        input |= (1 << 0);
    if (IsKeyDown(KEY_DOWN))
        input |= (1 << 1);
    if (IsKeyDown(KEY_RIGHT))
        input |= (1 << 2);
    if (IsKeyDown(KEY_LEFT))
        input |= (1 << 3);
    if (IsKeyDown(KEY_SPACE))
        input |= (1 << 4);

    return input;
}