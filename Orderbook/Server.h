#pragma once

#include "Orderbook.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <boost/asio/strand.hpp>
#include <thread>
#include <iostream>
#include <string>
#include <sstream>
#include <memory>

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http;   // from <boost/beast/http.hpp>
namespace net = boost::asio;    // from <boost/asio.hpp>
using tcp = net::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

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
        acceptor_.async_accept(
            net::make_strand(io_context_),
            beast::bind_front_handler(
                &Server::OnAccept,
                this));
    }

    void OnAccept(beast::error_code ec, tcp::socket socket)
    {
        if (!ec)
        {
            std::make_shared<Session>(std::move(socket), orderbook_)->Run();
        }
        StartAccept();
    }

    class Session : public std::enable_shared_from_this<Session>
    {
    public:
        Session(tcp::socket socket, Orderbook& orderbook)
            : socket_(std::move(socket)), orderbook_(orderbook) {}

        void Run()
        {
            DoRead();
        }

    private:
        void DoRead()
        {
            req_ = {};

            http::async_read(socket_, buffer_, req_,
                beast::bind_front_handler(
                    &Session::OnRead,
                    shared_from_this()));
        }

        void OnRead(beast::error_code ec, std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);

            if (ec == http::error::end_of_stream)
                return DoClose();

            if (ec)
                return;

            HandleRequest();
        }

        void HandleRequest()
        {
            if (req_.method() == http::verb::options)
            {
                // Handle CORS preflight request
                http::response<http::empty_body> res{ http::status::no_content, req_.version() };
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(http::field::access_control_allow_origin, "*");
                res.set(http::field::access_control_allow_methods, "POST, GET, OPTIONS");
                res.set(http::field::access_control_allow_headers, "Content-Type");
                res.prepare_payload();
                return SendResponse(std::move(res));
            }

            auto response_body = HandleCommand(req_.body());
            auto const size = response_body.size();

            auto res = std::make_shared<http::response<http::string_body>>(
                http::status::ok, req_.version());
            res->set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res->set(http::field::content_type, "text/plain");
            res->set(http::field::access_control_allow_origin, "*"); // Allow any origin
            res->content_length(size);
            res->keep_alive(req_.keep_alive());
            res->body() = std::move(response_body);

            http::async_write(
                socket_, *res,
                [self = shared_from_this(), res](beast::error_code ec, std::size_t bytes_transferred) {
                    self->OnWrite(res->need_eof(), ec, bytes_transferred, res);
                });
        }

        void SendResponse(http::response<http::empty_body>&& res)
        {
            auto self = shared_from_this();
            auto sp = std::make_shared<http::response<http::empty_body>>(std::move(res));
            http::async_write(socket_, *sp,
                [self, sp](beast::error_code ec, std::size_t) {
                    self->OnWrite(sp->need_eof(), ec, 0, sp);
                });
        }

        void OnWrite(bool close, beast::error_code ec, std::size_t bytes_transferred, const std::shared_ptr<http::response<http::string_body>>& res)
        {
            std::cout << "OnWrite()" << std::endl;
            boost::ignore_unused(bytes_transferred);

            if (ec)
            {
                std::cerr << "Error on write: " << ec.message() << std::endl;
                return;
            }

            if (close)
            {
                DoClose();
            }
            else
            {
                DoRead();
            }
        }

        void OnWrite(bool close, beast::error_code ec, std::size_t bytes_transferred, const std::shared_ptr<http::response<http::empty_body>>& res)
        {
            std::cout << "OnWrite() (empty body)" << std::endl;
            boost::ignore_unused(bytes_transferred);

            if (ec)
            {
                std::cerr << "Error on write: " << ec.message() << std::endl;
                return;
            }

            if (close)
            {
                DoClose();
            }
            else
            {
                DoRead();
            }
        }

        void DoClose()
        {
            beast::error_code ec;
            socket_.shutdown(tcp::socket::shutdown_send, ec);
            if (ec)
            {
                std::cerr << "Error on shutdown: " << ec.message() << std::endl;
            }
        }

        std::string HandleCommand(const std::string& received_data)
        {
            std::cout << "Received data: " << received_data << std::endl;
            std::istringstream stream(received_data);
            std::string stringType, stringOrderId, stringPrice, stringQuantity, stringOrderType;

            if (!(stream >> stringType >> stringOrderId >> stringPrice >> stringQuantity >> stringOrderType)) {
                std::cerr << "Failed to parse received data: " << received_data << std::endl;
                return "Error: Failed to parse command\n";
            }

            Side side;
            OrderId orderId = std::stoul(stringOrderId);
            Price price = std::stoul(stringPrice);
            Quantity quantity = std::stoul(stringQuantity);
            OrderType orderType = OrderType::GoodTillCancel;

            if (stringType == "B") {
                side = Side::Buy;
            }
            else {
                side = Side::Sell;
            }

            std::shared_ptr<Order> orderPointer = std::make_shared<Order>(orderType, orderId, side, price, quantity);
            orderbook_.AddOrder(orderPointer);

            return orderbook_.GetOrderInfos().PrintData();
        }

        tcp::socket socket_;
        beast::flat_buffer buffer_;
        http::request<http::string_body> req_;
        Orderbook& orderbook_;
    };

    Orderbook& orderbook_;
    net::io_context io_context_;
    tcp::acceptor acceptor_;
};
