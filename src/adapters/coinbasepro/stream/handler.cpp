#include "cryptoconnect/adapters/coinbasepro/stream/handler.hpp"

#include "cryptoconnect/helpers/utils/datetime.hpp"
#include "cryptoconnect/structs/events.hpp"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <string>

namespace CryptoConnect::CoinbasePro::Stream
{
    void Handler::onMessage(std::string const &message)
    {
        rapidjson::Document document;
        document.Parse(message.c_str());

        auto type = std::string(document["type"].GetString());

        if (type == "subscriptions")
            std::cout << "subscription event: " << message << '\n';
        else if (type == "snapshot")
            this->handleSnapshot(document);
        else if (type == "l2update") // Tick
            this->handleTick(document);
        else if (type == "ticker")
            this->handleTrade(document); // Trade
        else if (type == "received")
            this->handleOrderReceipt(document); // Order Status (received)
        else if (type == "open")
            this->handleOrderOpen(document); // Order Status (open)
        else if (type == "done")
            this->handleOrderDone(document); // Order Status (done)
        else if (type == "match")
            this->handleOrderMatch(document); // Transaction
        else if (type == "error")
            std::cerr << "Error encountered: " << message << '\n';
        else
            std::cout << "Unrecognized event: " << message << '\n';
    }

    void Handler::handleSnapshot(document_t &document)
    {
        /**
         * https://docs.cloud.coinbase.com/exchange/docs/channels#the-level2-channel
         *
         * Sample snapshot:
         * {
         *   "type": "snapshot",
         *   "product_id": "BTC-USD",
         *   "bids": [["10101.10", "0.45054140"], ...], // [price, size]
         *   "asks": [["10102.55", "0.57753524"], ...]  // [price, size]
         * }
         *
         * We use simdjson::ondemand for this since the JSON is ginormous
         */
        try
        {
            // Read the product ID
            auto productId = std::string(document["product_id"].GetString());
            auto bestAskPrice = std::stod(document["asks"].GetArray()[0].GetArray()[0].GetString());
            auto bestAskVolume = std::stod(document["asks"].GetArray()[0].GetArray()[1].GetString());
            auto bestBidPrice = std::stod(document["bids"].GetArray()[0].GetArray()[0].GetString());
            auto bestBidVolume = std::stod(document["bids"].GetArray()[0].GetArray()[1].GetString());

            // Update the best tick detail for the given product
            this->tickTracker_[productId] = Events::Tick(
                0, productId, bestBidPrice, bestAskPrice, bestBidVolume, bestAskVolume, true);
        }
        catch (std::exception const &e)
        {
            std::cerr << "Failed to parse snapshot: " << e.what() << '\n';
        }
    }

    void Handler::handleTick(document_t &document)
    {
        /**
         * https://docs.cloud.coinbase.com/exchange/docs/channels#the-level2-channel
         * 
         * Sample l2update:
         * {
         *   "type": "l2update",
         *   "product_id": "BTC-USD",
         *   "time": "2019-08-14T20:42:27.265Z",
         *   "changes": [["buy", "10101.80000000", "0.162567"]] // [side, price, quantityLeft] <-- not quantity delta
         * }
         */
        try
        {
            // Read the product ID
            auto productId = std::string(document["product_id"].GetString());

            // Guard-clause against updates where we do not have the snapshot taken
            if (this->tickTracker_.find(productId) == this->tickTracker_.end())
                return;

            // NOTE: Changes Array follows: [[SIDE (buy/sell), Updated Best Bid/Ask, Updated best Bid/Ask volume]]
            //       The volume represents the residual volume and not the change in volume
            //       Docs at: https://docs.cloud.coinbase.com/exchange/docs/channels#the-level2-channel
            auto side = std::string(document["changes"].GetArray()[0].GetArray()[0].GetString());
            auto updatedPrice = std::stod(document["changes"].GetArray()[0].GetArray()[1].GetString());
            auto updatedVolume = std::stod(document["changes"].GetArray()[0].GetArray()[2].GetString());

            // Pull out the reference to the current tick in the map
            Events::Tick &currentTick = this->tickTracker_[productId];

            // Read the timestamp and updating the tracker
            currentTick.epochTime_ = Utils::Datetime::isostringToEpoch<std::chrono::nanoseconds>(
                std::string(document["time"].GetString()));

            if (side == "buy")
            {
                // Buy-side update --> Best bid updated (retain previous best ask)
                currentTick.bid_ = updatedPrice;
                currentTick.volBid_ = updatedVolume;
                currentTick.isBuySide_ = true;
            }
            else
            {
                // Sell-side update --> Best ask updated (retain previous best bid)
                currentTick.ask_ = updatedPrice;
                currentTick.volAsk_ = updatedVolume;
                currentTick.isBuySide_ = false;
            }

            // Enqueue the event
            this->eventQueue_->enqueue<Events::Tick>(currentTick);
        }
        catch (std::exception const &e)
        {
            std::cerr << "Failed to parse tick: " << e.what() << '\n';
        }
    }

    void Handler::handleTrade(document_t &document)
    {
        /**
         * https://docs.cloud.coinbase.com/exchange/docs/channels#the-ticker-channel
         *
         * Sample:
         * {
         *   "type": "ticker",
         *   "trade_id": 20153558,
         *   "sequence": 3262786978,
         *   "time": "2017-09-02T17:05:49.250000Z",
         *   "product_id": "BTC-USD",
         *   "price": "4388.01000000",
         *   "side": "buy", // taker side
         *   "last_size": "0.03000000",
         *   "best_bid": "4388",
         *   "best_ask": "4388.01"
         * }
         */
        try
        {
            // Enqueue the event
            this->eventQueue_->enqueue<Events::Trade>(
                Utils::Datetime::isostringToEpoch<std::chrono::nanoseconds>(
                    document["time"].GetString()),
                document["product_id"].GetString(),
                std::stod(document["price"].GetString()),
                std::stod(document["last_size"].GetString()),
                std::string(document["side"].GetString()) == "buy");
        }
        catch (std::exception const &e)
        {
            std::cerr << "Failed to parse trade: " << e.what() << '\n';
        }
    }

    void Handler::handleOrderReceipt(document_t &document)
    {
        /**
         * https://docs.cloud.coinbase.com/exchange/docs/channels#received
         *
         * Market Order Sample:
         * Receipt:
         * {
         *   "order_id":"ee463aed-7a5d-4bb3-a320-5695f2ef7646",
         *   "order_type":"market",
         *   "size":"1.23",
         *   "funds":"55693.6840965687",
         *   "client_oid":"",
         *   "type":"received",
         *   "side":"buy",
         *   "product_id":"BTC-USD",
         *   "time":"2022-01-07T13:56:17.598637Z",
         *   "sequence":473096645,
         *   "profile_id":"3b39df7a-3aaf-4ffc-8d54-94f06f957764",
         *   "user_id":"5ddbb26138514d05fd3f7ac3"
         * }
         */
        // Track the order id
        auto orderId = document["order_id"].GetString();
        this->myOrderIds_.emplace(std::string(orderId));

        // Enqueue the event
        this->eventQueue_->enqueue<Events::OrderStatus>(
            document["order_id"].GetString(),
            Utils::Datetime::isostringToEpoch<std::chrono::nanoseconds>(
                document["time"].GetString()),
            document["product_id"].GetString(),
            Orders::Status::RECEIVED,
            std::stod(document["size"].GetString()));
    }

    /* Status update that order is open and still in the book */
    void Handler::handleOrderOpen(document_t &document)
    {
        /**
         * https://docs.cloud.coinbase.com/exchange/docs/channels#open
         *
         * Sample:
         * {
         *   "price":"49000",
         *   "order_id":"55d0ee7d-c3cc-4df0-835e-eabd6d369de6",
         *   "remaining_size":"0.01",
         *   "type":"open",
         *   "side":"sell",
         *   "product_id":"BTC-USD",
         *   "time":"2022-01-07T16:16:58.080622Z",
         *   "sequence":473171430,
         *   "profile_id":"3b39df7a-3aaf-4ffc-8d54-94f06f957764",
         *   "user_id":"5ddbb26138514d05fd3f7ac3"
         * }
         */

        // Events::OrderStatus orderStatus();

        // Feed the strategy
        this->eventQueue_->enqueue<Events::OrderStatus>(
            document["order_id"].GetString(),
            Utils::Datetime::isostringToEpoch<std::chrono::nanoseconds>(
                document["time"].GetString()),
            document["product_id"].GetString(),
            Orders::Status::OPEN,
            std::stod(document["remaining_size"].GetString()));
    }

    /* Status update that order is done */
    void Handler::handleOrderDone(document_t &document)
    {
        /**
         * https://docs.cloud.coinbase.com/exchange/docs/channels#done
         *
         * Sample:
         * {
         *   "order_id":"ee463aed-7a5d-4bb3-a320-5695f2ef7646",
         *   "reason":"filled",
         *   "remaining_size":"0",
         *   "type":"done",
         *   "side":"buy",
         *   "product_id":"BTC-USD",
         *   "time":"2022-01-07T13:56:17.598637Z",
         *   "sequence":473096669,
         *   "profile_id":"3b39df7a-3aaf-4ffc-8d54-94f06f957764",
         *   "user_id":"5ddbb26138514d05fd3f7ac3"
         * }
         */

        auto orderId = std::string(document["order_id"].GetString());

        // Remove the id from our map and feed the strategy
        this->myOrderIds_.erase(orderId);

        // Enqueue the event
        this->eventQueue_->enqueue<Events::OrderStatus>(
            orderId,
            Utils::Datetime::isostringToEpoch<std::chrono::nanoseconds>(
                document["time"].GetString()),
            document["product_id"].GetString(),
            Orders::Status::DONE, 0);
    }

    /* Transaction occured */
    void Handler::handleOrderMatch(document_t &document)
    {
        /**
         * https://docs.cloud.coinbase.com/exchange/docs/channels#match
         *
         * Status:
         * {
         *   "trade_id":37335538,
         *   "maker_order_id":"06269235-6144-47e4-8df3-cb2574d27c01",
         *   "taker_order_id":"ee463aed-7a5d-4bb3-a320-5695f2ef7646",
         *   "size":"0.14777874",
         *   "price":"45576",
         *   "taker_profile_id":"3b39df7a-3aaf-4ffc-8d54-94f06f957764",
         *   "taker_user_id":"5ddbb26138514d05fd3f7ac3",
         *   "taker_fee_rate":"0.005",
         *   "type":"match",
         *   "side":"sell",
         *   "product_id":"BTC-USD",
         *   "time":"2022-01-07T13:56:17.598637Z",
         *   "sequence":473096668,
         *   "profile_id":"3b39df7a-3aaf-4ffc-8d54-94f06f957764",
         *   "user_id":"5ddbb26138514d05fd3f7ac3"
         * }
         */

        auto makerOrderId = std::string(document["maker_order_id"].GetString());
        auto takerOrderId = std::string(document["taker_order_id"].GetString());
        bool isMaker = this->myOrderIds_.find(makerOrderId) != this->myOrderIds_.end();

        // Feed the strategy
        this->eventQueue_->enqueue<Events::Transaction>(
            isMaker ? makerOrderId : takerOrderId,
            Utils::Datetime::isostringToEpoch<std::chrono::nanoseconds>(
                document["time"].GetString()),
            document["product_id"].GetString(),
            std::stod(document["price"].GetString()),
            std::stod(document["size"].GetString()));
    }
}
