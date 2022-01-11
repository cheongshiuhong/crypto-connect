#include "adapters/coinbasepro/adapter.hpp"

#include "structs/events.hpp"
#include "structs/orders.hpp"
#include "structs/products.hpp"
#include "structs/universe.hpp"
#include "helpers/utils/exceptions.hpp"
#include "adapters/base.hpp"
#include "adapters/coinbasepro/auth.hpp"
#include "adapters/coinbasepro/rest/connector.hpp"
#include "adapters/coinbasepro/rest/bars_scheduler.hpp"
#include "adapters/coinbasepro/stream/connector.hpp"
#include "adapters/coinbasepro/stream/handler.hpp"

#include <thread>

namespace CryptoConnect::CoinbasePro
{
    Adapter::Adapter(BaseStrategy *strategy)
        : BaseAdapter(strategy){};

    void Adapter::start()
    {
        // Make the connection
        this->streamConnector_.connect();
        this->strategy_->onStart();

        // Use a separate thread for the recurring bar queries
        std::thread queryingThread(
            [this]
            {
                Utils::Exceptions::withHandler(
                    [this]
                    { this->barsScheduler_.queryBarsForever(); },
                    [this]
                    { this->strategy_->onExit(); },
                    "[ERROR] Scheduler bars querying failed.");
            });

        // Use a separate thread for the stream
        std::thread streamingThread(
            [this]
            {
                Utils::Exceptions::withHandler(
                    [this]
                    { this->streamConnector_.streamForever(this->streamHandler_); },
                    [this]
                    { this->strategy_->onExit(); },
                    "[ERROR] Stream connector failed.");
            });

        // Use the main thread for feeding the strategy
        this->feedStrategyForever();

        // We shall never reach here
        queryingThread.join();
        streamingThread.join();
    }

    void Adapter::getAvailableUniverse(Universe::Universe &output)
    {
        this->restConnector_.getProducts(this->productMap_, output);
    }

    void Adapter::getCurrentUniverse(Universe::Universe &output)
    {
        output.update(this->currentUniverse_);
    }

    Products::productPtr_t Adapter::lookupProductDetails(securityId_t const productId)
    {
        if (this->productMap_.find(productId) == this->productMap_.end())
            return nullptr;
        return this->productMap_[productId];
    }

    void Adapter::updateUniverse(Universe::Universe const &universe)
    {
        this->streamConnector_.unsubscribeProducts(this->currentUniverse_);
        this->currentUniverse_.update(universe);
        this->streamConnector_.subscribeProducts(this->currentUniverse_);
    }

    void Adapter::getBars(std::string const &productId, char const *granularity,
                          std::string const &start, std::string const &end,
                          Events::bars_t &output)
    {
        this->restConnector_.getBars(productId, granularity, start, end, output);
    }

    void Adapter::placeOrder(
        Orders::LimitOrder const &order,
        Orders::OrderResponse &output)
    {
        this->restConnector_.placeOrder(order, output);
    }

    void Adapter::placeOrder(
        Orders::MarketOrder const &order,
        Orders::OrderResponse &output)
    {
        this->restConnector_.placeOrder(order, output);
    }

    void Adapter::getOrder(std::string const &orderId, Orders::OrderDetails &output)
    {
        this->restConnector_.getOrder(orderId, output);
    }

    void Adapter::getAllOrders(
        Orders::Status const status,
        Orders::ordersDetails_t &output)
    {
        this->restConnector_.getAllOrders(status, output);
    }

    void Adapter::getAllOrders(
        std::string const &productId,
        Orders::Status const status,
        Orders::ordersDetails_t &output)
    {
        this->restConnector_.getAllOrders(productId, status, output);
    }

    bool Adapter::cancelOrder(std::string const &orderId)
    {
        return this->restConnector_.cancelOrder(orderId);
    }

    void Adapter::cancelAllOrders(Orders::orderIds_t &output)
    {
        this->restConnector_.cancelAllOrders(output);
    }

    void Adapter::cancelAllOrders(
        std::string const &productId,
        Orders::orderIds_t &output)
    {
        this->restConnector_.cancelAllOrders(productId, output);
    }

    void Adapter::feedStrategyForever()
    {
        while (1)
        {
            Events::Event event;
            this->eventQueue_.dequeue(event);

            std::visit(
                Events::overloaded{
                    [this](Events::Bar &bar)
                    { this->strategy_->onBar(bar); },
                    [this](Events::Tick &tick)
                    { this->strategy_->onTick(tick); },
                    [this](Events::Trade &trade)
                    { this->strategy_->onTrade(trade); },
                    [this](Events::OrderStatus &orderStatus)
                    { this->strategy_->onOrderStatus(orderStatus); },
                    [this](Events::Transaction &transaction)
                    { this->strategy_->onTransaction(transaction); }},
                event);
        }
    }
}
