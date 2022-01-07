#include "adapters/coinbasepro/stream/handler.hpp"

#include "helpers/utils/datetime.hpp"
#include "structs/events.hpp"
#include "adapters/coinbasepro/adapter.hpp"

#include <simdjson/simdjson.h>

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <string>

namespace Adapters::CoinbasePro::Stream
{
    void Handler::onMessage(std::string const &message)
    {
        // Cast to padded string
        simdjson::padded_string paddedMessage = message;

        // Parse the message with ondemand iterator
        // Documents ARE iterators (ondemand)
        simdjson::ondemand::parser parser;
        document_t document = parser.iterate(paddedMessage);

        auto typeView = document["type"].get_string().value();

        if (typeView == "subscriptions")
            std::cout << "subscription event" << document << '\n';
        else if (typeView == "snapshot")
            this->handleSnapshot(document);
        else if (typeView == "l2update") // Tick
            this->handleTick(document);
        else if (typeView == "ticker")
            this->handleTrade(document); // Trade
        else if (typeView == "received")
            this->handleOrderReceipt(document); // Track the orderId
        else if (typeView == "open")
            this->handleOrderOpen(document); // Order Status (open)
        else if (typeView == "done")
            this->handleOrderDone(document); // Order Status (done)
        else if (typeView == "match")
            this->handleOrderMatch(document); // Transaction
        else if (typeView == "error")
            std::cerr << "Error encountered: " << document << '\n';
        else
            std::cout << "Unrecognized event: " << document << '\n';
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
            auto productId = std::string(document["product_id"].get_string().value());

            // Extract ONLY the best asks and bids
            // NOTE: 'asks' MUST go before 'bids' and 'price' MUST go before 'volume'
            //        since we are using [ondemand] and have to adhere to the order of the JSON response string
            auto bestAskPrice = document["asks"].get_array().at(0).get_array().at(0).get_double_in_string().value();
            auto bestAskVolume = document["asks"].get_array().at(0).get_array().at(1).get_double_in_string().value();
            auto bestBidPrice = document["bids"].get_array().at(0).get_array().at(0).get_double_in_string().value();
            auto bestBidVolume = document["bids"].get_array().at(0).get_array().at(1).get_double_in_string().value();

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
            auto productId = std::string(document["product_id"].get_string().value());

            // Guard-clause against updates where we do not have the snapshot taken
            if (this->tickTracker_.find(productId) == this->tickTracker_.end())
                return;

            // NOTE: Changes Array follows: [[SIDE (buy/sell), Updated Best Bid/Ask, Updated best Bid/Ask volume]]
            //       The volume represents the residual volume and not the change in volume
            //       Docs at: https://docs.cloud.coinbase.com/exchange/docs/channels#the-level2-channel
            auto side = document["changes"].get_array().at(0).get_array().at(0).get_string().value();
            auto updatedPrice = document["changes"].get_array().at(0).get_array().at(1).get_double_in_string().value();
            auto updatedVolume = document["changes"].get_array().at(0).get_array().at(2).get_double_in_string().value();

            // Guard-clause against 0-volume updates
            // (Only useful if we are tracking the order book)
            // TO REMOVE IF WE ARE TO TRACK ORDER BOOK
            if (updatedVolume == 0)
                return;

            // Pull out the reference to the current tick in the map
            Events::Tick &currentTick = this->tickTracker_[productId];

            // Read the timestamp and updating the tracker
            currentTick.epochTime_ = Utils::Datetime::isostringToEpoch<std::chrono::nanoseconds>(
                std::string(document["time"].get_string().value()));

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
            Events::Trade trade(
                Utils::Datetime::isostringToEpoch<std::chrono::nanoseconds>(
                    std::string(document["time"].get_string().value())),
                std::string(document["product_id"].get_string().value()),
                document["price"].get_double_in_string(),
                document["last_size"].get_double_in_string(), true);

            // Enqueue the event
            this->eventQueue_->enqueue<Events::Trade>(trade);
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
        auto orderId = document["order_id"].get_string().value();
        this->myOrderIds_.emplace(std::string(orderId));

        // Enqueue the event
        this->eventQueue_->enqueue<Events::OrderStatus>(
            std::string(document["order_id"].get_string().value()),
            Utils::Datetime::isostringToEpoch<std::chrono::nanoseconds>(
                std::string(document["time"].get_string().value())),
            std::string(document["product_id"].get_string().value()),
            Orders::Status::OPEN,
            document["size"].get_double_in_string().value());
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

        Events::OrderStatus orderStatus(
            std::string(document["order_id"].get_string().value()),
            Utils::Datetime::isostringToEpoch<std::chrono::nanoseconds>(
                std::string(document["time"].get_string().value())),
            std::string(document["product_id"].get_string().value()),
            Orders::Status::OPEN,
            document["remaining_size"].get_double_in_string().value());

        // Feed the strategy
        this->eventQueue_->enqueue<Events::OrderStatus>(orderStatus);
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

        auto orderId = std::string(document["order_id"].get_string().value());

        // Remove the id from our map and feed the strategy
        this->myOrderIds_.erase(orderId);

        // Enqueue the event
        this->eventQueue_->enqueue<Events::OrderStatus>(
            orderId,
            Utils::Datetime::isostringToEpoch<std::chrono::nanoseconds>(
                std::string(document["time"].get_string().value())),
            std::string(document["product_id"].get_string().value()),
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

        auto makerOrderId = std::string(document["maker_order_id"].get_string().value());
        auto takerOrderId = std::string(document["taker_order_id"].get_string().value());
        bool isMaker = this->myOrderIds_.find(makerOrderId) != this->myOrderIds_.end();

        // Feed the strategy
        this->eventQueue_->enqueue<Events::Transaction>(
            isMaker ? makerOrderId : takerOrderId,
            Utils::Datetime::isostringToEpoch<std::chrono::nanoseconds>(
                std::string(document["time"].get_string().value())),
            std::string(document["product_id"].get_string().value()),
            document["price"].get_double_in_string().value(),
            document["size"].get_double_in_string().value());
    }
}
