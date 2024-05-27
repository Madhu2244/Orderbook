#include "Orderbook.h"
#include "Order.h"
#include <memory>
#include "Server.h"
#include "Client.h"
#include <thread>

void ServerThread(Orderbook& orderbook, int port)
{
    Server server = Server(orderbook, port);
    server.Run();
}

int main()
{
    Orderbook orderbook;

    std::thread serverThread(ServerThread, std::ref(orderbook), 8080);
    serverThread.detach();

    Client client = Client("localhost", "8080");
    client.Run();


    return 0;
}
