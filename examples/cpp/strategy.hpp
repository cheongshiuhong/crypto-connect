#ifndef STRATEGY_H
#define STRATEGY_H

#include "adapters/coinbasepro/adapter.hpp"
#include "strategies/base.hpp"

#include "./downloader.hpp"
#include "./filter.hpp"

#include <unordered_map>
#include <vector>

/* Dummy struct to hold a mean-reversion trading pair */
struct MRTradingPair
{
    std::string pair1_;
    std::string pair2_;
    double hedgeRatio_; // y = y_1 - h * y_2

    MRTradingPair(std::string pair1, std::string pair2, double hedgeRatio)
        : pair1_(pair1), pair2_(pair2), hedgeRatio_(hedgeRatio){};
};

/* Implement the base strategy */
class CBProStrategy : public Strategies::Base::Strategy
{
private:
    /* Helpers */
    CBProUniverseFilter universeFilter_{this};
    CBProDownloader downloader_{this};

public:
    /* Called when the adapter has been initialized */
    void onInit();

    /* Called when the adapter is connected and starts*/
    void onStart();
    void onBar(Events::Bar bar);
    void onTick(Events::Tick tick);
    void onTrade(Events::Trade trade);
    void onOrderStatus(Events::OrderStatus orderStatus);
    void onTransaction(Events::Transaction transaction);
    void onExit();

    /* Data Structures */
    std::vector<MRTradingPair> mrTradingPairs_;
};

#endif
