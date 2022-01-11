#include "./strategy.hpp"

#include "structs/events.hpp"
#include "structs/products.hpp"
#include "structs/universe.hpp"
#include "helpers/utils/datetime.hpp"

#include "Python.h"
#include "pyhelpers/cointegration_api.h"
#include "pyhelpers/cointegration.h"
#include "pyhelpers/stationarity_api.h"
#include "pyhelpers/stationarity.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

/* Called when the adapter has been initialized */
void CBProStrategy::onInit()
{
    std::cout << "[On Init]" << '\n';

    // Do some initializing logic here (adapter is not yet connected to the exchange)
}

/* Called when the adapter is connected and starts*/
void CBProStrategy::onStart()
{
    std::cout << "[On Start]" << '\n';

    // We can get the available products and run them through some filtering logic
    Universe::Universe availableUniverse;
    this->adapter_->getAvailableUniverse(availableUniverse);

    // We can then first filter based on simple product details
    this->universeFilter_.filterDetails(availableUniverse);

    // Now we load some bar data
    std::unordered_map<std::string, Events::bars_t> barsDataMap;
    this->downloader_.downloadLatestBars(availableUniverse, barsDataMap);

    // We then pass the bar data to the price volume filter
    this->universeFilter_.filterTopPriceVolume(availableUniverse, barsDataMap);

    // Now we copy the universe into a vector for preprocessing logic
    std::vector<std::string> selectedUniverseVec;
    std::copy(availableUniverse.begin(), availableUniverse.end(), std::back_inserter(selectedUniverseVec));

    // Extract the close price from the bars
    std::unordered_map<std::string, std::vector<double>> closePriceData;
    for (auto const &item : availableUniverse)
    {
        closePriceData[item] = std::vector<double>(barsDataMap[item].size());
        std::transform(
            barsDataMap[item].begin(), barsDataMap[item].end(),
            std::back_inserter(closePriceData[item]), [](Events::Bar bar)
            { return bar.close_; });
    }

    /**
     * We can also use Python for some help,
     * using libraries like Cython to expose a function
     *
     * Ensure that the shell has access to the python version/dependencies
     * (e.g. global or virtual environment)
     */

    // Initialize Python Interpreter
    Py_Initialize();

    // Bring in our cdef-ed helpers
    PyRun_SimpleString("import sys; sys.path.insert(0, '')");
    import_pyhelpers__cointegration();
    import_pyhelpers__stationarity();

    // We then look for cointegrating pairs to trade on as an example of using Python in C++
    for (int idx1 = 0; idx1 < selectedUniverseVec.size(); idx1++)
    {
        for (int idx2 = idx1 + 1; idx2 < selectedUniverseVec.size(); idx2++)
        {
            // Flip the sign for the sake of convention (y = y1 - hy2)
            // where we expect h > 0 s.t. we long one and short one
            double hedgeRatio = -py_get_pair_cointegration_ratio(
                closePriceData[selectedUniverseVec[idx1]],
                closePriceData[selectedUniverseVec[idx2]]);

            // We only want to take opposing positions
            if (hedgeRatio <= 0)
                continue;

            // Test for stationary with the best candidate
            bool isStationary = py_check_stationary(
                closePriceData[selectedUniverseVec[idx1]],
                closePriceData[selectedUniverseVec[idx2]],
                hedgeRatio,
                0.02);

            // Record the pair in our data structure
            if (isStationary)
            {
                this->mrTradingPairs_.emplace_back(
                    selectedUniverseVec[idx1],
                    selectedUniverseVec[idx2],
                    hedgeRatio);
            }
        }
    }

    // Terminate Python Interpreter
    Py_Finalize();

    // Log out our selected trading pairs
    std::cout << "Trading Pairs" << '\n';
    for (auto const &item : this->mrTradingPairs_)
    {
        std::cout << item.pair1_ << " | " << item.pair2_ << '\n';
    }

    // Update our trading universe
    this->adapter_->updateUniverse(availableUniverse);
}

void CBProStrategy::onBar(Events::Bar bar)
{
    std::cout << "[On Bar] " << bar << '\n';

    // Do some stuff here
}

void CBProStrategy::onTick(Events::Tick tick)
{
    std::cout << "[On Tick] " << tick << '\n';

    // Do some stuff here
}

void CBProStrategy::onTrade(Events::Trade trade)
{
    std::cout << "[On Trade] " << trade << '\n';

    // Do some stuff here
}

void CBProStrategy::onOrderStatus(Events::OrderStatus orderStatus)
{
    std::cout << "[On OrderStatus] " << orderStatus << '\n';

    // Do some stuff here
}

void CBProStrategy::onTransaction(Events::Transaction transaction)
{
    std::cout << "[On Transaction] " << transaction << '\n';

    // Do some stuff here
}

void CBProStrategy::onExit()
{
    // Do some closing logic here
}
