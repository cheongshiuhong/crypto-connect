#ifndef ADAPTERS_COINBASEPRO_STREAM_CONNECTOR_H
#define ADAPTERS_COINBASEPRO_STREAM_CONNECTOR_H

#include "build_config.h"

#include "helpers/network/websockets/client.hpp"
#include "structs/universe.hpp"

#if IS_SANDBOX
#define COINBASEPRO_WS_ENDPOINT "ws-feed-public.sandbox.exchange.coinbase.com"
#else
#define COINBASEPRO_WS_ENDPOINT "ws-feed.exchange.coinbase.com"
#endif

/* Forward declarations */
namespace Adapters::CoinbasePro
{
    class Auth;

    namespace Stream
    {
        class Handler;
    }
}

namespace Adapters::CoinbasePro::Stream
{
    class Connector
    {
    private:
        Network::WebSockets::Client wsClient_;
        Auth *auth_;

    public:
        /* Constructor */
        Connector(Auth *auth);

        /* Connects the socket */
        void connect();

        /* Loops and calls the callback on receiving a message */
        void streamForever(Handler &handler);

        /* Subscribes to products */
        void subscribeProducts(Universe::Universe const &universe);

        /* Unsubscribes to products */
        void unsubscribeProducts(Universe::Universe const &universe);

    private:
        /* Constructs the subscription message from the type and the unvierse */
        void makeSubscriptionMessage(
            std::string const type,
            Universe::Universe const &universe,
            std::string &output);

        /* Keeps the socket alive by pinging */
        void keepAlive();
    };
}

#endif
