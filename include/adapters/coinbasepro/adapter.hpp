#ifndef ADAPTERS_COINBASEPRO_ADAPTER_H
#define ADAPTERS_COINBASEPRO_ADAPTER_H

#include "build_config.h"

#include "strategies/base.hpp"
#include "structs/events.hpp"
#include "structs/orders.hpp"
#include "structs/products.hpp"
#include "structs/universe.hpp"
#include "../base.hpp"
#include "./auth.hpp"
#include "./rest/connector.hpp"
#include "./rest/bars_scheduler.hpp"
#include "./stream/connector.hpp"
#include "./stream/handler.hpp"

#include <mutex>
#include <string>
#include <variant>

namespace Adapters::CoinbasePro
{
    class Adapter : public Adapters::Base::Adapter
    {
    private:
        /* Tracks the current subscribed universe */
        Universe::Universe currentUniverse_;

        /* Tracks the products details */
        Products::productMap_t productMap_;

        /* Event queue for helpers to enqueue into and strategy to read from */
        Events::Queue eventQueue_;

        /* Auth headers generator */
        Auth auth_;

        /* REST Connector */
        REST::Connector restConnector_{&this->auth_};

        /* Bars Scheduler */
        REST::BarsScheduler barsScheduler_{&this->restConnector_, &this->eventQueue_, &this->currentUniverse_};

        /* Stream Connector and handler */
        Stream::Connector streamConnector_{&this->auth_};
        Stream::Handler streamHandler_{this, &this->eventQueue_};

    public:
        /* Constructor */
        Adapter(Strategies::Base::Strategy *strategy);

        void start();

        /* Products */
        void getAvailableUniverse(Universe::Universe &output);
        void updateUniverse(Universe::Universe const &universe);
        Products::productPtr_t lookupProductDetails(securityId_t const productId);

        /* Orders */
        void placeOrder(Orders::LimitOrder const &order, Orders::OrderResponse &output);
        void placeOrder(Orders::MarketOrder const &order, Orders::OrderResponse &output);

        void getOrder(std::string const &orderId, Orders::OrderDetails &output);
        void getAllOrders(Orders::Status const status, Orders::ordersDetails_t &output);
        void getAllOrders(std::string const &productId,
                          Orders::Status const status,
                          Orders::ordersDetails_t &output);

        bool cancelOrder(std::string const &orderId);
        void cancelAllOrders(Orders::orderIds_t &output);
        void cancelAllOrders(std::string const &productId, Orders::orderIds_t &output);

    private:
        /* Event feeding */
        void feedStrategyForever();
    };
}

#endif
