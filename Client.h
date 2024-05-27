#pragma once

#include <boost/asio.hpp>
#include <iostream>
#include <string>

using boost::asio::ip::tcp;

class Client
{
public:
    Client(const std::string& host, const std::string& port)
        : io_context_(), socket_(io_context_)
    {
        try
        {
            tcp::resolver resolver(io_context_);
            tcp::resolver::results_type endpoints = resolver.resolve(host, port);

            boost::asio::connect(socket_, endpoints);
        }
        catch (std::exception& e)
        {
            std::cerr << "Exception: " << e.what() << std::endl;
        }
    }

    void Run()
    {
        try
        {
            std::string message;
            while (std::getline(std::cin, message))
            {
                if (message == "exit")
                    break;

                SendMessage(message);
                std::string reply = ReceiveReply();
                std::cout << "Reply: " << reply << std::endl;
            }
        }
        catch (std::exception& e)
        {
            std::cerr << "Exception: " << e.what() << std::endl;
        }
    }

private:
    void SendMessage(const std::string& message)
    {
        boost::asio::write(socket_, boost::asio::buffer(message));
    }

    std::string ReceiveReply()
    {
        char reply[1024];
        size_t reply_length = socket_.read_some(boost::asio::buffer(reply, 1024));
        return std::string(reply, reply_length);
    }

    boost::asio::io_context io_context_;
    tcp::socket socket_;
};