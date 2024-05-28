#include <iostream>
#include <map>
#include <set>
#include <list>
#include <cmath>
#include <ctime>
#include <deque>
#include <queue>
#include <stack>
#include <limits>
#include <string>
#include <vector>
#include <numeric>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <memory>
#include <variant>
#include <optional>
#include <tuple>
#include <format>
#include <chrono>
#include <condition_variable>
enum class OrderType
{
    GoodTillCancel,
    FillOrKill
};

enum class Side
{
    Buy,
    Sell
};

using Price = std::int32_t;
using Quantity = std::uint32_t;
using OrderId = std::uint64_t;

struct LevelInfo
{
    Price price_;
    Quantity quantity_;
};

using LevelInfos = std::vector<LevelInfo>;

class OrderBookLevelInfos
{
    public:
        OrderBookLevelInfos(const LevelInfos& bids, const LevelInfos& asks)
            : bids_ { bids }
            , asks_ { asks }
        { }

        const LevelInfos& GetBids() const { return bids_; }
        const LevelInfos& GetAsks() const { return asks_; }

    private:
        LevelInfos bids_;
        LevelInfos asks_;
};

class Order
{
    public:
        Order(OrderType orderType, OrderId orderId, Side side, Price price, Quantity quantity)
            : orderType_ { orderType }
            , orderId_ { orderId }
            , side_ { side }
            , price_ { price }
            , initialQuantity_ { quantity }
            , remainingQuantity_ { quantity }
        { }

        OrderType GetOrderType() const { return orderType_; }
        OrderId GetOrderId() const { return orderId_; }
        Side GetSide() const { return side_; }
        Price GetPrice() const { return price_; }
        Quantity GetInitialQuantity() const { return initialQuantity_; }
        Quantity GetRemainingQuantity() const { return remainingQuantity_; }
        Quantity GetFilledQuantity() const { return GetInitialQuantity() - GetRemainingQuantity(); }
        void Fill (Quantity quantity) {
            if (quantity > GetRemainingQuantity()) {
                throw std::logic_error(std::format("Order ({}) cannot be filled for more than its remianing quantity.", GetOrderId()));
            }

            remainingQuantity_ -= quantity;
        }
        bool IsFilled() const { return GetRemainingQuantity() == 0; }


    private:
        OrderType orderType_;
        OrderId orderId_;
        Side side_;
        Price price_;
        Quantity initialQuantity_;
        Quantity remainingQuantity_;
};

using OrderPointer = std::shared_ptr<Order>;
using OrderPointers = std::list<OrderPointer>;

class OrderModify
{
    public:
        OrderModify(OrderId orderId, Side side, Price price, Quantity quantity)
            : orderId_ { orderId }
            , side_ { side }
            , price_ { price }
            , quantity_ { quantity }
        { }

        OrderId GetOrderId() const { return orderId_; }
        Side GetSide() const { return side_; }
        Price GetPrice() const { return price_; }
        Quantity GetQuantity() const { return quantity_; }
        
        OrderPointer ToOrderPointer(OrderType type) const
        {
            return std::make_shared<Order>(type, GetOrderId(), GetPrice(), GetQuantity());
        }
    
    private:
        OrderId orderId_;
        Side side_;
        Price price_;
        Quantity quantity_;
};

struct TradeInfo
{
    OrderId orderId_;
    Price price_;
    Quantity quantity_;
};


class Trade
{
    public:
        Trade(const TradeInfo& bidTrade, const TradeInfo& askTrade)
            : bidTrade_ { bidTrade }
            , askTrade_ { askTrade }
        { }

        const TradeInfo& GetBidTrade() const { return bidTrade_; }
        const TradeInfo& GetAskTrade() const { return askTrade_; }

    private:
        TradeInfo bidTrade_;
        TradeInfo askTrade_;
};

using Trades = std::vector<Trade>;

class Orderbook
{
    private:
        struct OrderEntry
        {
            OrderPointer order_ { nullptr };
            OrderPointers::iterator location_;
        };

        std::map<Price, OrderPointers, std::greater<Price>> bids_;
        std::map<Price, OrderPointers, std::less<Price>> asks_;
        std::unordered_map<OrderId, OrderEntry> orders_;

        bool CanMatch(Side side, Price price) const 
        {
            if (side == Side::Buy)
            {
                if (asks_.empty())
                    return false;
                
                const auto& [bestAsk, _] = *asks_.begin();
                return price >= bestAsk;
            }
            else
            {
                if (bids_.empty())
                    return false;
                
                const auto& [bestBid, _] = *bids_.begin();
                return price <= bestBid;
            }
        }

        Trades MatchOrders() {
            Trades trades;
            trades.reserve(orders_.size());

            while (!bids_.empty() && !asks_.empty()) {
                auto& [bidPrice, bids] = *bids_.begin();
                auto& [askPrice, asks] = *asks_.begin();

                if (bidPrice < askPrice)
                    break;

                while (!bids.empty() && !asks.empty()) {
                    auto& bid = bids.front();
                    auto& ask = asks.front();

                    // Check for Fill or Kill conditions before matching
                    if ((bid->GetOrderType() == OrderType::FillOrKill && bid->GetRemainingQuantity() > asks.size()) ||
                        (ask->GetOrderType() == OrderType::FillOrKill && ask->GetRemainingQuantity() > bids.size())) {
                        CancelOrder(bid->GetOrderId());
                        CancelOrder(ask->GetOrderId());
                        bids.pop_front();
                        asks.pop_front();
                        continue;
                    }

                    Quantity quantity = std::min(bid->GetRemainingQuantity(), ask->GetRemainingQuantity());

                    bid->Fill(quantity);
                    ask->Fill(quantity);

                    if (bid->IsFilled()) {
                        bids.pop_front();
                        orders_.erase(bid->GetOrderId());
                    }

                    if (ask->IsFilled()) {
                        asks.pop_front();
                        orders_.erase(ask->GetOrderId());
                    }

                    trades.push_back(Trade { 
                        TradeInfo{ bid->GetOrderId(), bid->GetPrice(), quantity },
                        TradeInfo{ ask->GetOrderId(), ask->GetPrice(), quantity }
                    });

                    if (bids.empty())
                        bids_.erase(bidPrice);
                    if (asks.empty())
                        asks_.erase(askPrice);
                }
            }

            return trades;
        }
    public:
        Trades AddOrder(OrderPointer order)
        {
            if (orders_.contains(order->GetOrderId()))
                return { };
            if (order->GetOrderType() == OrderType::FillOrKill && !CanMatch(order->GetSide(), order->GetPrice()))
                return { };
            
            OrderPointers::iterators iterator;

            if (order->GetSide() == Side::buy)
            {
                auto& orders = bids_[order->GetPrice()];
                orders.push_back(order);
                iterator = std::next(orders.begin(), orders.size() - 1);
            }
            else
            {
                auto& orders = asks_[order->GetPrice()];
                orders.push_back(order);
                iterator = std::next(orders.begin(), orders.size() - 1);
            }

            orders_.insert({ order->GetOrderId(), OrderEntry { order, iterator} });
            return MatchOrders();

        }

        void CancelOrder(OrderId orderId)
        {
            if (!orders_.contains(orderId))
                return;
            
            const auto& [order, orderIterator] = orders_.at(orderId);
            orders_.erase(orderId);

            if (order->GetSide == Side::Sell)
            {
                auto price = order->GetPrice();
                auto& orders = asks_.at(price);
                orders.erase(iterator);
                if (orders.empty())
                    asks_.erase(price);
            }
            else
            {
                auto price = order->GetPrice();
                auto& orders = bids_.at(price);
                orders.erase(iterator);
                if(orderds.empty())
                    bids_.erase(price);
            }
        }

        Trades MarchOrder(OrderModify order)
        {
            if (!orders_.contains(orders.GetOrderId()))
                return { };
            
            const auto& [existingOrder, _] = orders_.at(order.GetOrderId());
            CancelOrder(order.GetOrderId());
            return AddOrder(order.ToOrderPointer(existingOrder->GetOrderType()));
        }

        std::size_t Size() const { return orders_.size(); }

        OrderBookLevelInfos GetOrderInfos() const
        {
            LevelInfos bidInfos, askInfos;
            bidInfos.reserve(orders_.size());
            askInfos.reserve(orders_.size());

            auto CreateLevelInfos = [](Price price, const OrderPointers& orders)
            {
                return LevelInfo{ price, std::accumulate(orders.begin(), orders.end(), (Quantity)0,
                    [](std::size_t, runningSum, const OrderPointer& order)
                    { return runningSum + order->GetRemainingQuantity(); }) };
            };

            for (const auto& [price, orders] : bids_)
                bidInfos.push_back(CreateLevelInfos(price, orders));
            
            for (const auto& [price, orders] : asks_)
                askInfos.push_back(CreateLevelInfos(price, orders));

            return OrderbookLevelInfos{ bidInfos, askInfos };
            
        }
};


int main()
{
    Orderbook orderbook;

    return 0;
}
