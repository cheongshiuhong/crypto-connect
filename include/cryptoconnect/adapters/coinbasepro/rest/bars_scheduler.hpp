#ifndef CRYPTOCONNECT_COINBASEPRO_REST_BARSSCHEDULER_H
#define CRYPTOCONNECT_COINBASEPRO_REST_BARSSCHEDULER_H

/* Use only 2 threads in sandbox to minimize chances of exceeding rate-limit */
#if IS_SANDBOX
#define BAR_QUERY_THREADS 2
#else
#define BAR_QUERY_THREADS 8
#endif

#include "cryptoconnect/structs/events.hpp"
#include "cryptoconnect/structs/event_queue.hpp"
#include "cryptoconnect/structs/universe.hpp"

#include <cstdint>
#include <chrono>

/* Forward declarations */
namespace CryptoConnect::CoinbasePro::REST
{
    class Connector;
}

namespace CryptoConnect::CoinbasePro::REST
{
    /**
     * Makes the REST calls for bars
     * 
     * We need this because CoinbasePro doesn't offer a channel
     * to get pinged for bars.
     */
    class BarsScheduler
    {
    private:
        REST::Connector *restConnector_;
        Universe::Universe *currentUniverse_;
        Events::Queue *eventQueue_;

        /* Tracks the current minute */
        uint64_t currentMinute_{0};

    public:
        /* Constructor */
        BarsScheduler(REST::Connector *restConnector, Events::Queue *eventQueue,
                      Universe::Universe *currentUniverse)
            : restConnector_(restConnector), eventQueue_(eventQueue),
              currentUniverse_(currentUniverse){};

        /* Method to be ran in a sleeping thread to initiate the queries in a thread pool */
        void queryBarsForever();

    private:
        /* Method to query for the bars and enqueue in the stream handler */
        void makeBarQueries();
    };
}

#endif
