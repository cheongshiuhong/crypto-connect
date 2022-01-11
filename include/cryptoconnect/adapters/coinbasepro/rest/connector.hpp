#ifndef CRYPTOCONNECT_COINBASEPRO_HTTP_CONNECTOR_H
#define CRYPTOCONNECT_COINBASEPRO_HTTP_CONNECTOR_H

#include "build_config.h"

#include "helpers/network/http/session.hpp"
#include "structs/events.hpp"
#include "structs/orders.hpp"
#include "structs/products.hpp"
#include "structs/universe.hpp"

#include <rapidjson/document.h>

#include <string>
#include <unordered_set>

#if IS_SANDBOX
#define COINBASEPRO_REST_ENDPOINT "api-public.sandbox.exchange.coinbase.com"
#else
#define COINBASEPRO_REST_ENDPOINT "api.exchange.coinbase.com"
#endif

/* Forward declarations */
namespace CryptoConnect::CoinbasePro
{
    class Auth;
}

namespace CryptoConnect::CoinbasePro::REST
{
    class Connector
    {
        /* Allow friend BarsScheduler to query raw bars */
        friend class BarsScheduler;

    private:
        Network::HTTP::Session publicSession_{COINBASEPRO_REST_ENDPOINT, "443"};
        Network::HTTP::Session privateSession_{COINBASEPRO_REST_ENDPOINT, "443"};
        Auth *auth_;
        unsigned long uniqueOrderId_{0};

    public:
        Connector(Auth *auth);

        /* Products */
        void getProducts(Products::productMap_t &productMapOutput,
                         Universe::Universe &availableUniverseOutput);
        void getBars(std::string const &productId, char const *granularity,
                     std::string const &start, std::string const &end,
                     Events::bars_t &output);
        void getMinuteBars(std::string const &productId, std::string const &start,
                           std::string const &end, Events::bars_t &output);
        void getDailyBars(std::string const &productId, std::string const &start,
                          std::string const &end, Events::bars_t &output);

        /* Orders */
        void placeOrder(Orders::LimitOrder const &order, Orders::OrderResponse &output);
        void placeOrder(Orders::MarketOrder const &order, Orders::OrderResponse &output);

        void getOrder(std::string const &orderId, Orders::OrderDetails &output);
        void getAllOrders(Orders::Status const status, Orders::ordersDetails_t &output);
        void getAllOrders(std::string const &produtId,
                          Orders::Status const status,
                          Orders::ordersDetails_t &output);

        bool cancelOrder(std::string const &orderId);
        void cancelAllOrders(Orders::orderIds_t &output);
        void cancelAllOrders(std::string const &productId, Orders::orderIds_t &output);

    private:
        void getRawBars(std::string const &productId, char const *granularity,
                        std::string const &start, std::string const &end,
                        std::string &output);

        void parseOrderResponse(std::string const &orderResponseString,
                                Orders::OrderResponse &output);

        void parseOrderDetails(rapidjson::Value const &obj, Orders::OrderDetails &output);
    };
}

#endif
