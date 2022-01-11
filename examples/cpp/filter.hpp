#ifndef UNIVERSE_FILTER_H
#define UNIVERSE_FILTER_H

#include "structs/universe.hpp"
#include "structs/events.hpp"

#include <unordered_map>

/* Forward declarations */
class CBProStrategy;

class CBProUniverseFilter
{
private:
    CBProStrategy *strategy_;

public:
    CBProUniverseFilter(CBProStrategy *strategy);
    void filterDetails(Universe::Universe &universe);
    void filterTopPriceVolume(
        Universe::Universe &universe,
        std::unordered_map<std::string, Events::bars_t> barsDataMap);
};

#endif
