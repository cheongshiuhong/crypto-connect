#ifndef CRYPTOCONNECT_BASESTRATEGY_H
#define CRYPTOCONNECT_BASESTRATEGY_H

#include "structs/events.hpp"
#include "adapters/base.hpp"

#include <iostream>
#include <chrono>

namespace CryptoConnect
{
    class BaseStrategy
    {
    public:
        BaseAdapter *adapter_;

        inline void registerAdapter(BaseAdapter *adapter)
        {
            this->adapter_ = adapter;
        }

        /* Interface */
        virtual void onInit() = 0;
        virtual void onStart() = 0;
        virtual void onBar(Events::Bar bar) = 0;
        virtual void onTick(Events::Tick tick) = 0;
        virtual void onTrade(Events::Trade trade) = 0;
        virtual void onOrderStatus(Events::OrderStatus orderStatus) = 0;
        virtual void onTransaction(Events::Transaction transaction) = 0;
        virtual void onExit() = 0;
    };
}

#endif
