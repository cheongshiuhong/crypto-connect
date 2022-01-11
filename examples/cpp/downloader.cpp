#include "./downloader.hpp"

#include "./strategy.hpp"

#include "structs/events.hpp"
#include "structs/universe.hpp"

#include <boost/asio/thread_pool.hpp>

CBProDownloader::CBProDownloader(CBProStrategy *strategy) : strategy_(strategy){};

void CBProDownloader::downloadLatestBars(
    Universe::Universe const &universe,
    std::unordered_map<std::string, Events::bars_t> &barsDataMap)
{
    constexpr int numDays = 3;
    constexpr int numBars = 288 * numDays; // 288 x 5 mins per day
    uint64_t timeNow = Utils::Datetime::epochNow<std::chrono::seconds>();

    // Public endpoint rate limit is 3 req/sec for CoinbasePro
    boost::asio::thread_pool pool(3);
    for (auto const productId : universe)
    {
        // Emplace an empty vector into the map
        barsDataMap.emplace(productId, Events::bars_t());
        barsDataMap[productId].reserve(numBars);

        boost::asio::post(
            pool,
            [this, timeNow, productId, &barsDataMap]
            {
                uint64_t queryEndTime = timeNow;
                // Get 7 days of data
                for (int j = 0; j < numDays; j++)
                {
                    this->strategy_->adapter_->getBars(
                        productId,
                        "300",                                                   // 5-min bars
                        Utils::Datetime::epochToIsostring(queryEndTime - 86100), // - 23hrs 55mins
                        Utils::Datetime::epochToIsostring(queryEndTime),
                        barsDataMap[productId]);
                    queryEndTime -= 86400;
                }
            });
    }
    pool.join();
}
