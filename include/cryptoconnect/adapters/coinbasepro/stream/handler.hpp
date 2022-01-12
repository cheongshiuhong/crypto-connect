#ifndef CRYPTOCONNECT_COINBASEPRO_STREAM_HANDLER_H
#define CRYPTOCONNECT_COINBASEPRO_STREAM_HANDLER_H

#include "cryptoconnect/helpers/utils/datetime.hpp"
#include "cryptoconnect/structs/events.hpp"
#include "cryptoconnect/structs/event_queue.hpp"

#include <rapidjson/document.h>

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace CryptoConnect::CoinbasePro::Stream
{
    using document_t = rapidjson::Document;

    class Handler
    {
    private:
        Events::Queue *eventQueue_;

        /**
         * We need to track the snapshots and previous ticks for each security
         * since l2updates only provide the updated bid/ask side's info
         */
        std::unordered_map<std::string, Events::Tick> tickTracker_;

        /** We need to track our order IDs */
        std::unordered_set<std::string> myOrderIds_;

    public:
        /* Constructor */
        Handler(Events::Queue *eventQueue) : eventQueue_(eventQueue){};

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
