#ifndef STRUCTS_ORDERES_H
#define STRUCTS_ORDERES_H

#include "build_config.h"

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace Orders
{
    enum class Type
    {
        MARKET = 0,
        LIMIT = 1,
        UNKNOWN = 99
    };

    enum class Side
    {
        BUY = 0,
        SELL = 1,
        UNKONWN = 99
    };

    enum class Status
    {
        RECEIVED = 0,
        OPEN = 1,
        DONE = 2,
        UNKNOWN = 99
    };

    struct LimitOrder
    {
        Side side_;
        securityId_t securityId_;
        double price_;
        double quantity_;

        LimitOrder(Side side, securityId_t securityId, double price, double quantity)
            : side_(side), securityId_(securityId), price_(price), quantity_(quantity){};
    };

    struct MarketOrder
    {
        Side side_;
        securityId_t securityId_;
        double quantity_;

        MarketOrder(Side side, securityId_t securityId, double quantity)
            : side_(side), securityId_(securityId), quantity_(quantity){};
    };

    /* Order Id Types */
    using orderId_t = std::string;
    using orderIds_t = std::vector<orderId_t>;

    struct OrderResponse
    {
        enum class Code
        {
            SUCCESS = 0,
            UNAUTHORIZED = 1,
            INSUFFICIENT_FUNDS = 2,
            INVALID_PRODUCT = 3,
            UNFORESEEN_FAILURE = 4,
            EMPTY = 99
        };
        orderId_t id_;
        Code code_;

        OrderResponse()
            : id_(""), code_(Code::EMPTY){};

        OrderResponse(orderId_t id, Code code)
            : id_(id), code_(code){};
    };

    inline std::ostream &operator<<(std::ostream &os, OrderResponse &orderResponse)
    {
        switch (orderResponse.code_)
        {
        case OrderResponse::Code::EMPTY:
            os << "[EMPTY] Order response has not been initialized";
            break;
        case OrderResponse::Code::SUCCESS:
            os << "[SUCCESS] Order ID: " << orderResponse.id_;
            break;
        case OrderResponse::Code::UNAUTHORIZED:
            os << "[UNAUTHORIZED]";
            break;
        case OrderResponse::Code::INSUFFICIENT_FUNDS:
            os << "[INSUFFICIENT FUNDS]";
            break;
        case OrderResponse::Code::INVALID_PRODUCT:
            os << "[INVALID PRODUCT]";
            break;
        case OrderResponse::Code::UNFORESEEN_FAILURE:
            os << "[UNFORESEEN ERROR]";
            break;
        }

        return os;
    }

    struct OrderDetails
    {
        orderId_t id_;
        Type type_;
        Side side_;
        Status status_;
        uint64_t epochTime_;
        securityId_t securityId_;
        double price_;
        double quantity_;
        double quantityFilled_;
        double fees_;

        OrderDetails()
            : id_(""), type_(Type::UNKNOWN), side_(Side::UNKONWN), status_(Status::UNKNOWN),
              epochTime_(0), securityId_(""), price_(0),
              quantity_(0), quantityFilled_(0), fees_(0){};

        OrderDetails(orderId_t id, Type type, Side side, Status status,
                     uint64_t epochTime, securityId_t securityId, double price,
                     double quantity, double quantityFilled, double fees)
            : id_(id), type_(type), side_(side), status_(status),
              epochTime_(epochTime), securityId_(securityId), price_(price),
              quantity_(quantity), quantityFilled_(quantityFilled), fees_(fees){};
    };

    /* Details Types */
    using orderDetailsPtr_t = std::shared_ptr<OrderDetails>;
    using ordersDetails_t = std::vector<OrderDetails>;
}

#endif
