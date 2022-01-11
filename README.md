# crypto-connect

An opinionated project for connecting to Crypto Exchanges in C++20.

## Summary

This project aims to provide a common interface between the different crypto exchanges (more to come), with an opinionated approach as to what event streams are consumed and what orders can be made (GTC-only).

It is strongly advised to heavily test it in the sandbox before using it in live.


## Dependencies

1. Boost (v1.71.0+)
2. libssl-dev

## Features

- [x] Retrieve Available Products and Product Details
- [x] Market Data Stream (Bars, Ticks, Trades, OrderStatuses, Transactions)
- [x] Historical Bar Data Queries
- [x] Order Placing (GTC-only) for Market and Limit (non-margin)
- [x] Order Cancellation
- [x] Order Tracking (both as streamed event and querying it directly with REST)
- [ ] Accounts


## Build (CMake on the way)

This project is developed in Linux and should work on Unix-based systems.

Build the shared object with the relevant `include/cryptoconnect/build_config.h` and link it (this is where it should be specified if it is for the sandbox or for live).

```shell
$ make cbpro
```


## Usage

1. Include the header files `include/cryptoconnect`.

2. Then, include the adapter and implement the base strategy to handle the market events.

3. A `config.yaml` file is also expected for the provision of API keys, secrets, and passphrases as seen in [config.template.yaml](config.template.yaml).

```c++
#include "cryptoconnect/adapters/coinbasepro.hpp"
#include "cryptoconnect/strategy.hpp"
#include "cryptoconnect/structs/events.hpp"
#include "cryptoconnect/structs/orders.hpp"
#include "cryptoconnect/structs/universe.hpp"

class MyStrategy : public CryptoConnect::BaseStrategy
{
public:
    void onInit()
    {
        // Some initializing logic here
    }

    void onStart()
    {
        // Logic to filter and set the trading universe
        Universe::Universe availableUniverse;
        this->adapter_->getAvailableUniverse(availableUniverse);
        ...
        Universe::Universe myUniverse({"BTC-USD", "ETH-USD"});
        this->adapter_->updateUniverse(myUniverse);
    }

    void onBar(Events::Bar bar)
    {
        // Do something when a new minute bar is aggregated
    }

    void onTick(Events::Tick tick)
    {
        // Do something when the best bid/ask is updated
    }

    void onTrade(Events::Trade trade)
    {
        // Do something when there is a trade/match in the market
        if (trade.securityId_ == "BTC-USD" && trade.lastPrice_ < 40000)
        {
            Orders::MarketOrder order(Orders::Side::BUY, "BTC-USD", 1.23);
            Orders::OrderResponse response;
            this->adapter_.placeOrder(order, response);
            std::cout << response << '\n';
        }
    }

    void onOrderStatus(Events::OrderStatus orderStatus)
    {
        // Track my order statuses (received/open/done)
    }

    void onTransaction(Events::Transaction transaction)
    {
        // Get notified when my order matches
    }

    void onExit()
    {
        // Some closing logic here
    }
};

int main()
{
    MyStrategy myStrategy;
    CryptoConnect::CoinbasePro::Adapter adapter(&myStrategy);

    adapter.start();
}
```
More details details can be found in the [examples](examples/cpp).

## Future developments

1. To test the stability of the project and improve error handling
2. More exchanges
3. Possible extension for use in Python


## Contributors

- [Cheong Shiu Hong](https://github.com/cheongshiuhong)
