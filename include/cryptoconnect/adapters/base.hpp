#ifndef CRYPTOCONNECT_BASEADAPTER_H
#define CRYPTOCONNECT_BASEADAPTER_H

#include "build_config.h"

#include "structs/events.hpp"
#include "structs/orders.hpp"
#include "structs/products.hpp"
#include "structs/universe.hpp"

/* Forward declarations */
namespace CryptoConnect
{
    class BaseStrategy;
}

namespace CryptoConnect
{
    class BaseAdapter
    {
    protected:
        /* Constructor */
        BaseAdapter(BaseStrategy *const strategy);

        /* Pointer to the strategy instance */
        BaseStrategy *strategy_;

    public:
        /* Interface */
        virtual void start() = 0;

        virtual void getAvailableUniverse(Universe::Universe &output) = 0;
        virtual void getCurrentUniverse(Universe::Universe &output) = 0;
        virtual void updateUniverse(Universe::Universe const &universe) = 0;
        virtual Products::productPtr_t lookupProductDetails(securityId_t const productId) = 0;

        virtual void getBars(std::string const &productId, char const *granularity,
                             std::string const &start, std::string const &end,
                             Events::bars_t &output) = 0;

        virtual void placeOrder(Orders::LimitOrder const &order, Orders::OrderResponse &output) = 0;
        virtual void placeOrder(Orders::MarketOrder const &order, Orders::OrderResponse &output) = 0;

        virtual void getOrder(std::string const &orderId, Orders::OrderDetails &output) = 0;
        virtual void getAllOrders(Orders::Status const status, Orders::ordersDetails_t &output) = 0;
        virtual void getAllOrders(std::string const &productId,
                                  Orders::Status const status,
                                  Orders::ordersDetails_t &output) = 0;

        virtual bool cancelOrder(std::string const &orderId) = 0;
        virtual void cancelAllOrders(Orders::orderIds_t &output) = 0;
        virtual void cancelAllOrders(std::string const &productId, Orders::orderIds_t &output) = 0;
    };
}

#endif
