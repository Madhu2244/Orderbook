#include "Orderbook.h"
#include "Order.h"
#include <memory>
#include "Server.h"
#include "Client.h"
#include <thread>

int main()
{
    Orderbook orderbook;

    Server server = Server(orderbook, 8080);
    server.Run();


    return 0;
}
