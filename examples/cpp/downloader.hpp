#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include "cryptoconnect/structs/events.hpp"
#include "cryptoconnect/structs/universe.hpp"

#include <string>
#include <unordered_map>

/* Forward declarations */
class CBProStrategy;

class CBProDownloader
{
private:
    CBProStrategy *strategy_;

public:
    CBProDownloader(CBProStrategy *strategy);
    void downloadLatestBars(Universe::Universe const &universe,
                            std::unordered_map<std::string, Events::bars_t> &barsDataMap);
};

#endif
