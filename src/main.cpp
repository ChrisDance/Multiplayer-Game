#include "Server.hpp"
#include "Client.hpp"

int main(int argc, char **argv)
{

    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " server|client [client_port]\n";
        return 1;
    }

    int serverPort = 5050;

    if (strcmp(argv[1], "server") == 0)
    {

        Server server(serverPort);
        server.Attach();
        server.Run();
    }
    else if (strcmp(argv[1], "client") == 0 && argc > 2)
    {

        int clientPort = atoi(argv[2]);

        Client client(clientPort, serverPort);
        client.Attach();
        client.Run();
    }
    else
    {
        std::cerr << "Invalid arguments. Use 'server' or 'client [port]'\n";
        return 1;
    }

    return 0;
}
