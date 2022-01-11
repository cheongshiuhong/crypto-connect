#include "./filter.hpp"

#include "./strategy.hpp"

#include "structs/events.hpp"
#include "structs/universe.hpp"
#include "helpers/utils/datetime.hpp"

#include <boost/asio/thread_pool.hpp>

#include <algorithm>
#include <cstdint>
#include <chrono>
#include <unordered_map>
#include <vector>

#define DOLLAR_VOLUME_PERCENTILE 0.2

CBProUniverseFilter::CBProUniverseFilter(CBProStrategy *strategy)
{
    this->strategy_ = strategy;
}

void CBProUniverseFilter::filterDetails(Universe::Universe &universe)
{
    Universe::Universe universeToRemove;
    for (auto const &item : universe)
    {
        // e.g., we only want USD-quoted products & price increment > 0.001 (to avoid precision hell)
        auto ptr = this->strategy_->adapter_->lookupProductDetails(item);
        if (!ptr->isTradingEnabled_ || ptr->quoteCurrency_ != "USD" || ptr->quoteIncrement_ < 0.001)
            universeToRemove.emplace(item);
    }

    for (auto const &item : universeToRemove)
        universe.erase(item);
}

void CBProUniverseFilter::filterTopPriceVolume(
    Universe::Universe &universe,
    std::unordered_map<std::string, Events::bars_t> barsDataMap)
{
    std::unordered_map<std::string, double> priceVolumesMap;

    for (auto const &pair : barsDataMap)
    {
        double priceVolume = 0;
        for (auto const &bar : std::get<1>(pair))
        {
            priceVolume += (bar.close_ * bar.vol_);
        }
        priceVolumesMap.emplace(std::get<0>(pair), priceVolume);
    }

    // Extract the top nth products
    int nToExtract = barsDataMap.size() * DOLLAR_VOLUME_PERCENTILE;
    std::vector<std::string> universeVec;
    std::transform(barsDataMap.begin(), barsDataMap.end(), back_inserter(universeVec),
                   [](std::pair<std::string, Events::bars_t> pair)
                   { return pair.first; });
    std::partial_sort(universeVec.begin(), universeVec.begin() + nToExtract, universeVec.end(),
                      [&](std::string id1, std::string id2)
                      { return priceVolumesMap[id1] > priceVolumesMap[id2]; });

    // Write the top N into the universe
    for (int i = nToExtract; i < universeVec.size(); i++)
        universe.erase(universeVec[i]);
}
