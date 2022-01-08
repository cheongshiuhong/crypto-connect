#include "adapters/coinbasepro/rest/bars_scheduler.hpp"

#include "helpers/utils/datetime.hpp"
#include "structs/events.hpp"
#include "structs/universe.hpp"
#include "adapters/coinbasepro/rest/connector.hpp"

#include <boost/asio/thread_pool.hpp>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <cstdint>
#include <chrono>
#include <thread>

namespace Adapters::CoinbasePro::REST
{
    /**
     * We will query bars at the 5th second of every minute
     * 
     * Will still have missing bars for low-volume products (depending on if there were trades in an interval)
     */
    void BarsScheduler::queryBarsForever()
    {
        // Track the minute now
        this->currentMinute_ = std::chrono::duration_cast<std::chrono::minutes>(
                                   std::chrono::system_clock::now().time_since_epoch())
                                   .count();

        // Scoped operations for the initial query
        {
            // Check how many milliseconds we are past the latest minute and the query first
            auto currentMinuteInMillisecond = this->currentMinute_ * Utils::Constants::c_msInMinute;
            auto currentMillisecond = std::chrono::duration_cast<std::chrono::milliseconds>(
                                          std::chrono::system_clock::now().time_since_epoch())
                                          .count();
            auto millisecondsPastLatestMinute = currentMillisecond - currentMinuteInMillisecond;

            // Allow to be signed (milliseconds is fine)
            int64_t millisecondsBefore10thSecond = (10000 - millisecondsPastLatestMinute);
            if (millisecondsBefore10thSecond > 0)
            {
                // If we are before the 10th second of the minute, wait first
                std::this_thread::sleep_for(std::chrono::milliseconds(millisecondsBefore10thSecond));

                // Update the current millisecond if we slept
                currentMillisecond += millisecondsBefore10thSecond;
            }
            else if (millisecondsBefore10thSecond < -40000) // 50th second
            {
                // If we are close to the end of the minute, wait till the next 10th second
                std::this_thread::sleep_for(std::chrono::milliseconds(Utils::Constants::c_msInMinute + millisecondsBefore10thSecond));

                // Update the current millisecond if we slept
                currentMillisecond += millisecondsBefore10thSecond;

                // We are now in the next minute
                currentMinuteInMillisecond += Utils::Constants::c_msInMinute;
                this->currentMinute_++;
            }

            // Make the first query now that we are guaranteed to be past the 10th second
            std::thread initialQueryThread([&]
                                           { this->makeBarQueries(); });

            // Compute how many milliseconds we need to sleep in order to wake up near the 10th second in the next minute
            auto millisecondsToSleep = Utils::Constants::c_msInMinute - (currentMillisecond - currentMinuteInMillisecond) + 10000;

            std::this_thread::sleep_for(std::chrono::milliseconds(millisecondsToSleep));

            initialQueryThread.join();
            // End of scope --> Pop these numbers off the stack
        }

        // Loop and query every 60 seconds now that we're aligned at the 10th second
        while (1)
        {
            this->currentMinute_++;
            std::thread queryThread([&]
                                    { this->makeBarQueries(); });
            std::this_thread::sleep_for(std::chrono::seconds(60));
            queryThread.join();
        }
    }

    void BarsScheduler::makeBarQueries()
    {
        // CoinbasePro's bar time is given as the start of the aggregation interval (so we have to -1 here)
        std::string end = Utils::Datetime::epochToIsostring((this->currentMinute_ - 1) * Utils::Constants::c_sInMinute);
        std::string start = Utils::Datetime::epochToIsostring((this->currentMinute_ - 1) * Utils::Constants::c_sInMinute - 5); // just offset 5 seconds is enough

        boost::asio::thread_pool pool(BAR_QUERY_THREADS);
        for (auto const &productId : (*this->currentUniverse_))
        {
            boost::asio::post(
                pool,
                [&]()
                {
                    // Fetch the raw minute bars
                    std::string response;
                    this->restConnector_->getRawBars(
                        productId, "60", start, end, response);

                    // Parse the response
                    rapidjson::Document document;
                    document.Parse(response.c_str());

                    if (!document.IsArray())
                    {
                        std::cerr << "Invalid bars response for "
                                  << productId << " " << response << '\n';
                        return;
                    }

                    if (!document.GetArray().Size())
                    {
                        std::cerr << "No bars received for "
                                  << productId << " " << response << '\n';
                        return;
                    }

                    for (auto const &barJson : document.GetArray())
                    {
                        this->eventQueue_->enqueue<Events::Bar>(
                            (barJson[0].GetUint64() + 60) * 1000000000, // epoch time in nanoseconds (+1 since coinbase gives time as start of agg interval)
                            productId,                                  // securityId
                            barJson[3].GetDouble(),                     // open
                            barJson[2].GetDouble(),                     // high
                            barJson[1].GetDouble(),                     // low
                            barJson[4].GetDouble(),                     // close
                            barJson[5].GetDouble()                      // volume
                        );
                        break;
                    }
                });
        }
        pool.join();
    }
}
