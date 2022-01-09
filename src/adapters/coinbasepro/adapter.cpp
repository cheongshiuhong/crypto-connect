#include "adapters/coinbasepro/adapter.hpp"

#include "structs/events.hpp"
#include "structs/orders.hpp"
#include "structs/products.hpp"
#include "structs/universe.hpp"
#include "adapters/base.hpp"
#include "adapters/coinbasepro/auth.hpp"
#include "adapters/coinbasepro/rest/connector.hpp"
#include "adapters/coinbasepro/rest/bars_scheduler.hpp"
#include "adapters/coinbasepro/stream/connector.hpp"
#include "adapters/coinbasepro/stream/handler.hpp"

#include <thread>

namespace Adapters::CoinbasePro
{
    Adapter::Adapter(Strategies::Base::Strategy *strategy)
        : Adapters::Base::Adapter(strategy){};

    void Adapter::start()
    {
        // Make the connection
        this->streamConnector_.connect();
        this->strategy_->onStart();

        // Use a separate thread for the recurring bar queries
        std::thread queryingThread(
            [this]
            { this->barsScheduler_.queryBarsForever(); });

        // Use a separate thread for the stream
        std::thread streamingThread(
            [this]
            { this->streamConnector_.streamForever(this->streamHandler_); });

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
