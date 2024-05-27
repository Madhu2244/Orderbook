#pragma once

#include "LevelInfo.h"

class OrderbookLevelInfos
{
public:
    OrderbookLevelInfos(const LevelInfos& bids, const LevelInfos& asks)
        : bids_{ bids }
        , asks_{ asks }
    { }

    const LevelInfos& GetBids() const { return bids_; }
    const LevelInfos& GetAsks() const { return asks_; }

    const std::string PrintData() {
        std::string ret;

        ret += "Pending Bids:\n";
        for (auto [price, quantity] : bids_) {
           ret += std::format("{}:{}\n", std::to_string(price), std::to_string(quantity));
        }

        ret += "\nPending Asks:\n";
        for (auto [price, quantity] : asks_) {
            ret += std::format("{}:{}", std::to_string(price), std::to_string(quantity));
        }

        return ret;
    }

private:
    LevelInfos bids_;
    LevelInfos asks_;
};
