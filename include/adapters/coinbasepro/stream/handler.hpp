#ifndef ADAPTERS_COINBASEPRO_STREAM_HANDLER_H
#define ADAPTERS_COINBASEPRO_STREAM_HANDLER_H

#include "build_config.h"

#include "helpers/utils/datetime.hpp"
#include "structs/events.hpp"

#include <simdjson/simdjson.h>

#include <string>
#include <unordered_map>
#include <unordered_set>

/* Forward Declarations */
namespace Adapters::CoinbasePro
{
    class Adapter;
}

namespace Adapters::CoinbasePro::Stream
{
    using document_t = simdjson::ondemand::document;

    class Handler
    {
    private:
        Adapter *adapter_;
        Events::Queue *eventQueue_;

        /**
         * We need to track the snapshots and previous ticks for each security
         * since l2updates only provide the updated bid/ask side's info
         */
        std::unordered_map<securityId_t, Events::Tick> tickTracker_;

        /** We need to track our order IDs */
        std::unordered_set<std::string> myOrderIds_;

    public:
        /* Constructor */
        Handler(Adapter *adapter, Events::Queue *eventQueue)
            : adapter_(adapter), eventQueue_(eventQueue){};

        void onMessage(std::string const &message);

    private:
        void handleSnapshot(document_t &document);
        void handleBar(document_t &document);
        void handleTick(document_t &document);
        void handleTrade(document_t &document);
        void handleOrderReceipt(document_t &document);
        void handleOrderOpen(document_t &document);
        void handleOrderDone(document_t &document);
        void handleOrderMatch(document_t &document);
    };
}

#endif
