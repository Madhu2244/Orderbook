#pragma once

#include "Orderbook.h"
#include <boost/asio.hpp>
#include <thread>
#include <iostream>
#include <string>
#include <sstream>

using boost::asio::ip::tcp;
    
class Server
{
public:
    Server(Orderbook& orderbook, unsigned short port)
        : orderbook_{ orderbook }, acceptor_{ io_context_, tcp::endpoint(tcp::v4(), port) }
    {
        StartAccept();
    }

    void Run()
    {
        io_context_.run();
    }

private:
    void StartAccept()
    {
        auto socket = std::make_shared<tcp::socket>(io_context_);
        acceptor_.async_accept(*socket, [this, socket](boost::system::error_code ec)
            {
                if (!ec)
                {
                    std::thread([this, socket]() { HandleClient(socket); }).detach();
                }
                StartAccept();
            });
    }

    void HandleClient(std::shared_ptr<tcp::socket> socket)
    {
        try
        {
            for (;;)
            {
                char data[512];
                boost::system::error_code error;
                size_t length = socket->read_some(boost::asio::buffer(data), error);

                if (error == boost::asio::error::eof)
                    break; // Connection closed cleanly by peer.
                else if (error)
                    throw boost::system::system_error(error); // Some other error.

                std::string received_data(data, length);
                // Process received data and interact with the orderbook
                std::cout << "Received: " << received_data << std::endl;

                // For demonstration, let's just send back an acknowledgment
                std::string response = HandleCommand(received_data);
                std::cout << response << std::endl;
                
                std::cout << "Printed Data\n" << orderbook_.GetOrderInfos().PrintData() << std::endl;
                boost::asio::write(*socket, boost::asio::buffer(response));
            }
        }
        catch (std::exception& e)
        {
            std::cerr << "Exception: " << e.what() << "\n";
        }
    }

    std::string HandleCommand(std::string& received_data)
    {
        std::cout << "Handling command!" << std::endl;
        std::istringstream stream(received_data);
        std::string stringType, stringOrderId, stringPrice, stringQuantity, stringOrderType;

        if (!(stream >> stringType >> stringOrderId >> stringPrice >> stringQuantity >> stringOrderType)) {
            std::cerr << "Failed to parse received data: " << received_data << std::endl;
            return "Error: Failed to parse command\n";
        }

        std::cout << "Parsed data: " << stringType << " " << stringOrderId << " "
            << stringPrice << " " << stringQuantity << " " << stringOrderType << std::endl;

        
        Side side;
        OrderId orderId = std::stoul(stringOrderId);
        Price price = std::stoul(stringPrice);
        Quantity quantity = std::stoul(stringQuantity);
        OrderType orderType = OrderType::GoodTillCancel;

        if (stringType == "B") {
            side = Side::Buy;
        }
        else { // stringType == "S"
            side = Side::Sell;
        }

        std::shared_ptr<Order> orderPointer = std::make_shared<Order>(orderType, orderId, side, price, quantity);

        orderbook_.AddOrder(orderPointer);
                
        return "Added Order\n";


        // received data will be in the format
        // Type: B or S for Buy or Sell Side
        // OrderId: any numerical value
        // Price: Any numerical positive value
        // Quantity: Any numerical value
        // OrderType: Doesn't matter right now.
        // Example: S 1 100 1 GoodTillCancel
    }

    Orderbook& orderbook_;
    boost::asio::io_context io_context_;
    tcp::acceptor acceptor_;
};