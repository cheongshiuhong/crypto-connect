#include "adapters/coinbasepro/rest/connector.hpp"

#include "helpers/network/http/session.hpp"
#include "helpers/utils/datetime.hpp"
#include "structs/events.hpp"
#include "structs/orders.hpp"
#include "structs/products.hpp"
#include "structs/universe.hpp"
#include "adapters/coinbasepro/auth.hpp"

#include <boost/asio/thread_pool.hpp>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <future>
#include <memory>
#include <string>
#include <utility>

namespace CryptoConnect::CoinbasePro::REST
{
    Connector::Connector(Auth *auth) : auth_(auth)
    {
        // Add the auth request decorator to the private session
        this->privateSession_.addRequestDecorator(
            [this](Network::HTTP::request_t &req)
            { this->auth_->addAuthHeaders(req); });
    }

    void Connector::getProducts(
        Products::productMap_t &productMapOutput,
        Universe::Universe &availableUniverseOutput)
    {
        /**
         * https://docs.cloud.coinbase.com/exchange/reference/exchangerestapi_getproducts
         *
         * Sample response:
         * [
         *   {
         *     "id": "BTC-USD",
         *     "base_currency": "BTC",
         *     "quote_currency": "USD",
         *     "base_min_size": "0.00100000",
         *     "base_max_size": "280.00000000",
         *     "quote_increment": "0.01000000",
         *     "base_increment": "0.00000001",
         *     "display_name": "BTC/USD",
         *     "min_market_funds": "10",
         *     "max_market_funds": "1000000",
         *     "margin_enabled": false,
         *     "post_only": false,
         *     "limit_only": false,
         *     "cancel_only": false,
         *     "status": "online",
         *     "status_message": "",
         *     "auction_mode": true
         *   }
         * ]
         */

        std::string productsResponse;
        this->publicSession_.get("/products", productsResponse);

        // Parse the JSON response
        rapidjson::Document symbolsDocument;
        symbolsDocument.Parse(productsResponse.c_str());

        // Clear the existing product map
        // Shared pointers will de-allocate since map is no longer pointing to it
        productMapOutput.clear();

        // Iterate and emplace into the output obj
        for (auto const &productDetailsObj : symbolsDocument.GetArray())
        {
            auto productId = productDetailsObj["id"].GetString();

            auto productPtr = std::make_shared<Products::Product>(
                productId,
                productDetailsObj["display_name"].GetString(),
                productDetailsObj["base_currency"].GetString(),
                productDetailsObj["quote_currency"].GetString(),
                std::stod(productDetailsObj["base_min_size"].GetString()),
                std::stod(productDetailsObj["base_max_size"].GetString()),
                std::stod(productDetailsObj["base_increment"].GetString()),
                std::stod(productDetailsObj["quote_increment"].GetString()),
                productDetailsObj.HasMember("trading_disabled")
                    ? !productDetailsObj["trading_disabled"].GetBool()
                    : true,
                productDetailsObj["margin_enabled"].GetBool());

            // Register the product
            productMapOutput[productId] = productPtr;

            // Record the symbol as avaiable unviverse
            availableUniverseOutput.emplace(productId);
        }
    }

    void Connector::getMinuteBars(std::string const &productId, std::string const &start,
                                  std::string const &end, Events::bars_t &output)
    {
        this->getBars(productId, "60", start, end, output);
    }

    void Connector::getDailyBars(std::string const &productId, std::string const &start,
                                 std::string const &end, Events::bars_t &output)
    {
        this->getBars(productId, "86400", start, end, output);
    }

    void Connector::getBars(std::string const &productId, char const *granularity,
                            std::string const &start, std::string const &end,
                            Events::bars_t &output)
    {
        /**
         * https://docs.cloud.coinbase.com/exchange/reference/exchangerestapi_getproductcandles
         *
         * Sample Response:
         * [
         *   [
         *     1641471180, // epoch time in seconds
         *     42751.79,   // low
         *     42806.75,   // high
         *     42806.75,   // open
         *     42755.88,   // close
         *     8.25591271  // volume
         *   ], ...
         * ]
         */
        std::string response;
        this->getRawBars(productId, granularity, start, end, response);

        rapidjson::Document document;
        document.Parse(response.c_str());

        if (!document.IsArray())
        {
            std::cerr << "No bars received for " << productId << " | " << response << '\n';
            return;
        }

        for (auto const &barJson : document.GetArray())
            output.emplace_back(
                barJson[0].GetUint64() * 1000000000, // epoch time in nanoseconds
                productId,                           // securityId
                barJson[3].GetDouble(),              // open
                barJson[2].GetDouble(),              // high
                barJson[1].GetDouble(),              // low
                barJson[4].GetDouble(),              // close
                barJson[5].GetDouble()               // volume
            );
    }

    void Connector::getRawBars(std::string const &productId, char const *granularity,
                               std::string const &start, std::string const &end,
                               std::string &output)
    {
        std::string target = "/products/" + productId + "/candles?granularity=" + granularity + "&start=" + start + "&end=" + end;
        this->publicSession_.isolatedGet(target.c_str(), output);
    }

    void Connector::placeOrder(
        Orders::LimitOrder const &order,
        Orders::OrderResponse &output)
    {
        rapidjson::Document orderDocument;
        orderDocument.SetObject();
        auto orderAllocator = orderDocument.GetAllocator();

        rapidjson::Value productIdValue(rapidjson::kObjectType);
        productIdValue.SetString(order.securityId_.c_str(), order.securityId_.size(), orderAllocator);
        orderDocument.AddMember("product_id", productIdValue, orderAllocator);

        rapidjson::Value uniqueOrderIdValue(rapidjson::kObjectType);
        std::string uniqueOrderIdString = std::to_string(this->uniqueOrderId_++);
        uniqueOrderIdValue.SetString(uniqueOrderIdString.c_str(), uniqueOrderIdString.size(), orderAllocator);
        orderDocument.AddMember("client_oid", uniqueOrderIdValue, orderAllocator);

        orderDocument.AddMember("type", "limit", orderAllocator);

        if (order.side_ == Orders::Side::BUY)
            orderDocument.AddMember("side", "buy", orderAllocator);
        else
            orderDocument.AddMember("side", "sell", orderAllocator);

        rapidjson::Value priceValue(rapidjson::kObjectType);
        std::string priceString = std::to_string(order.price_);
        priceValue.SetString(priceString.c_str(), priceString.size(), orderAllocator);
        orderDocument.AddMember("price", priceValue, orderAllocator);

        rapidjson::Value quantityValue(rapidjson::kObjectType);
        std::string quantityString = std::to_string(order.quantity_);
        quantityValue.SetString(quantityString.c_str(), quantityString.size(), orderAllocator);
        orderDocument.AddMember("size", quantityValue, orderAllocator);

        // Serialize to string
        rapidjson::StringBuffer jsonStringBuffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(jsonStringBuffer);
        orderDocument.Accept(writer);

        // Assign the message and post it
        std::string orderBody = jsonStringBuffer.GetString();
        std::string orderResponseString;
        this->privateSession_.post("/orders", orderResponseString, orderBody);

        this->parseOrderResponse(orderResponseString, output);
    }

    void Connector::placeOrder(
        Orders::MarketOrder const &order,
        Orders::OrderResponse &output)
    {
        rapidjson::Document orderDocument;
        orderDocument.SetObject();
        auto orderAllocator = orderDocument.GetAllocator();

        rapidjson::Value productIdValue(rapidjson::kObjectType);
        productIdValue.SetString(order.securityId_.c_str(), order.securityId_.size(), orderAllocator);
        orderDocument.AddMember("product_id", productIdValue, orderAllocator);

        rapidjson::Value uniqueOrderIdValue(rapidjson::kObjectType);
        std::string uniqueOrderIdString = std::to_string(this->uniqueOrderId_++);
        uniqueOrderIdValue.SetString(uniqueOrderIdString.c_str(), uniqueOrderIdString.size(), orderAllocator);
        orderDocument.AddMember("client_oid", uniqueOrderIdValue, orderAllocator);

        orderDocument.AddMember("type", "market", orderAllocator);

        if (order.side_ == Orders::Side::BUY)
            orderDocument.AddMember("side", "buy", orderAllocator);
        else
            orderDocument.AddMember("side", "sell", orderAllocator);

        rapidjson::Value quantityValue(rapidjson::kObjectType);
        std::string quantityString = std::to_string(order.quantity_);
        quantityValue.SetString(quantityString.c_str(), quantityString.size(), orderAllocator);
        orderDocument.AddMember("size", quantityValue, orderAllocator);

        // Serialize to string
        rapidjson::StringBuffer jsonStringBuffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(jsonStringBuffer);
        orderDocument.Accept(writer);

        // Assign the message
        std::string orderBody = jsonStringBuffer.GetString();
        std::string orderResponseString;
        this->privateSession_.post("/orders", orderResponseString, orderBody);

        this->parseOrderResponse(orderResponseString, output);
    }

    void Connector::parseOrderResponse(
        std::string const &orderResponseString,
        Orders::OrderResponse &output)
    {
        /**
         * https://docs.cloud.coinbase.com/exchange/reference/exchangerestapi_postorders
         *
         * Sample Success Limit Response:
         * {
         *   "id":"d7aef8bc-832e-457f-a90f-1de60516b4d0",
         *   "price":"49000.000000",
         *   "size":"0.010000",
         *   "product_id":"BTC-USD",
         *   "side":"buy",
         *   "stp":"dc",
         *   "type":"limit",
         *   "time_in_force":"GTC",
         *   "post_only":false,
         *   "created_at":"2022-01-07T12:09:56.90717Z",
         *   "fill_fees":"0",
         *   "filled_size":"0",
         *   "executed_value":"0",
         *   "status":"pending",
         *   "settled":false
         * }
         * Sample Success Market Response:
         * {
         *   'id': 'c8c4effb-fb92-4413-8f03-af876f05757f',
         *   'size': '0.001',
         *   'product_id': 'BTC-USD',
         *   'side': 'sell',
         *   'stp': 'dc',
         *   'type': 'market',
         *   'post_only': False,
         *   'created_at': '2022-01-09T09:13:00.400937Z',
         *   'fill_fees': '0',
         *   'filled_size': '0',
         *   'executed_value': '0',
         *   'status': 'pending',
         *   'settled': False
         * }
         */
        rapidjson::Document responseDocument;
        responseDocument.Parse(orderResponseString.c_str());

        Orders::OrderResponse::Code orderResponseCode;

        // If there is a message --> failed
        if (responseDocument.HasMember("message"))
        {
            std::string message = responseDocument["message"].GetString();
            if (message == "Insufficient funds")
                orderResponseCode = Orders::OrderResponse::Code::INSUFFICIENT_FUNDS;
            else if (message == "product_id is not a valid product")
                orderResponseCode = Orders::OrderResponse::Code::INVALID_PRODUCT;
            else if (message == "Unauthorized." || message == "Invalid API Key" || message == "invalid signature" || message == "Invalid Passphrase" || message == "invalid timestamp")
                orderResponseCode = Orders::OrderResponse::Code::UNAUTHORIZED;
            else
            {
                orderResponseCode = Orders::OrderResponse::Code::UNFORESEEN_FAILURE;
                std::cerr << "Unforeseen order failure with message: " << message << "~" << '\n';
            }
            output = Orders::OrderResponse("", orderResponseCode);
            return;
        }

        // Construct the order response for a successful order
        output = Orders::OrderResponse(
            responseDocument["id"].GetString(),
            Orders::OrderResponse::Code::SUCCESS);
    }

    void Connector::getOrder(std::string const &orderId, Orders::OrderDetails &output)
    {
        /**
         * https://docs.cloud.coinbase.com/exchange/reference/exchangerestapi_getorder
         */

        std::string target = "/orders/" + orderId;
        std::string response;
        this->privateSession_.isolatedGet(target.c_str(), response);

        rapidjson::Document responseDocument;
        responseDocument.Parse(response.c_str());

        // If there is a message --> failed
        if (responseDocument.HasMember("message"))
        {
            std::cerr << "Failed to get order " << orderId
                      << " | Message: " << responseDocument["message"].GetString() << '\n';
            return;
        }

        // Auto-cast cast as a rapidjson::Value object
        this->parseOrderDetails(responseDocument, output);
    }

    void Connector::getAllOrders(
        Orders::Status const status,
        Orders::ordersDetails_t &output)
    {
        /**
         * https://docs.cloud.coinbase.com/exchange/reference/exchangerestapi_getorders
         */

        std::string target = "/orders";
        switch (status)
        {
        case Orders::Status::RECEIVED:
            target += "?status=received";
            break;
        case Orders::Status::OPEN:
            target += "?status=open";
            break;
        case Orders::Status::DONE:
            target += "?status=done";
            break;
            // Get all if unknown
        }

        std::string response;
        this->privateSession_.get(target.c_str(), response);

        rapidjson::Document responseDocument;
        responseDocument.Parse(response.c_str());

        // If it is an object --> failed
        if (responseDocument.IsObject())
        {
            std::cerr << "Failed to get orders  | Message: "
                      << responseDocument["message"].GetString() << '\n';
            return;
        }

        if (!responseDocument.IsArray())
        {
            std::cerr << "Invalid response received " << responseDocument.GetString() << '\n';
            return;
        }

        for (auto const &orderObj : responseDocument.GetArray())
        {
            // A little inefficient here (double construction)
            output.emplace_back();
            this->parseOrderDetails(orderObj, output.back());
        }
    }

    void Connector::getAllOrders(
        std::string const &productId,
        Orders::Status const status,
        Orders::ordersDetails_t &output)
    {
        /**
         * https://docs.cloud.coinbase.com/exchange/reference/exchangerestapi_getorders
         */

        std::string target = "/orders?productId=" + productId;
        switch (status)
        {
        case Orders::Status::RECEIVED:
            target += "&status=received";
            break;
        case Orders::Status::OPEN:
            target += "&status=open";
            break;
        case Orders::Status::DONE:
            target += "&status=done";
            break;
            // Get all if unknown
        }

        std::string response;
        this->privateSession_.get(target.c_str(), response);

        rapidjson::Document responseDocument;
        responseDocument.Parse(response.c_str());

        // If it is an object --> failed
        if (responseDocument.IsObject())
        {
            std::cerr << "Failed to get orders | Message: "
                      << responseDocument["message"].GetString() << '\n';
            return;
        }

        if (!responseDocument.IsArray())
        {
            std::cerr << "Invalid response received " << responseDocument.GetString() << '\n';
            return;
        }

        for (auto const &orderObj : responseDocument.GetArray())
        {
            // A little inefficient here (double construction)
            output.emplace_back();
            this->parseOrderDetails(orderObj, output.back());
        }
    }

    void Connector::parseOrderDetails(
        rapidjson::Value const &obj,
        Orders::OrderDetails &output)
    {
        /**
         * https://docs.cloud.coinbase.com/exchange/reference/exchangerestapi_getorder
         * 
         * Note: Market orders have price = null
         * 
         * Sample Response:
         * {
         *   'id': 'c8c4effb-fb92-4413-8f03-af876f05757f',
         *   'price': '49999',
         *   'size': '0.00100000',
         *   'product_id': 'BTC-USD',
         *   'side': 'sell',
         *   'type': 'limit',
         *   'created_at': '2022-01-09T09:13:00.400937Z',
         *   'fill_fees': '0.0000000000000000',
         *   'filled_size': '0.00000000',
         *   'executed_value': '0.0000000000000000',
         *   'status': 'done',
         *   'settled': False
         * }
         */

        bool isMarket = std::string(obj["type"].GetString()) == "market";
        std::string side = std::string(obj["side"].GetString());
        std::string status = std::string(obj["status"].GetString());

        output = Orders::OrderDetails(
            obj["id"].GetString(),
            isMarket
                ? Orders::Type::MARKET
                : Orders::Type::LIMIT,
            side == "buy"
                ? Orders::Side::BUY
                : Orders::Side::SELL,
            status == "open"
                ? Orders::Status::OPEN
                : (status == "done"
                       ? Orders::Status::DONE
                       : Orders::Status::UNKNOWN),
            Utils::Datetime::isostringToEpoch<std::chrono::nanoseconds>(
                obj["created_at"].GetString()),
            obj["product_id"].GetString(),
            isMarket
                ? 0
                : std::stod(obj["price"].GetString()),
            std::stod(obj["size"].GetString()),
            std::stod(obj["filled_size"].GetString()),
            std::stod(obj["fill_fees"].GetString()));
    }

    bool Connector::cancelOrder(std::string const &orderId)
    {
        /**
         * https://docs.cloud.coinbase.com/exchange/reference/exchangerestapi_deleteorder
         * 
         * Sample Response: (Just a plain string response)
         * "c8c4effb-fb92-4413-8f03-af876f05757f"
         */

        std::string target = "/orders/" + orderId;
        std::string response;
        this->privateSession_.isolatedDel(target.c_str(), response);

        rapidjson::Document responseDocument;
        responseDocument.Parse(response.c_str());

        // If there is a message --> failed
        if (responseDocument.HasMember("message"))
        {
            std::cerr << "Failed to cancel order " << orderId
                      << " | Message: " << responseDocument["message"].GetString() << '\n';
            return false;
        }
        return true;
    }

    void Connector::cancelAllOrders(Orders::orderIds_t &output)
    {
        /**
         * https://docs.cloud.coinbase.com/exchange/reference/exchangerestapi_deleteorders
         *
         * Sample Response:
         * [
         *   '5f3064bc-5adc-49cd-ae98-7ff81de21b3c',
         *   'bbadca39-447b-4f48-96b0-81d8800e4c07',
         *   '55d0ee7d-c3cc-4df0-835e-eabd6d369de6'
         * ]
         */
        std::string response;
        this->privateSession_.del("/orders", response);

        rapidjson::Document responseDocument;
        responseDocument.Parse(response.c_str());

        // If it is an object --> failed
        if (responseDocument.IsObject())
        {
            std::cerr << "Failed to get orders | Message: "
                      << responseDocument["message"].GetString() << '\n';
            return;
        }

        if (!responseDocument.IsArray())
        {
            std::cerr << "Invalid response received " << responseDocument.GetString() << '\n';
            return;
        }

        for (auto const &orderIdObj : responseDocument.GetArray())
        {
            output.emplace_back(orderIdObj.GetString());
        }
    }

    void Connector::cancelAllOrders(
        std::string const &productId,
        Orders::orderIds_t &output)
    {
        /**
         * https://docs.cloud.coinbase.com/exchange/reference/exchangerestapi_deleteorders
         * 
         * Sample Response:
         * [
         *   '5f3064bc-5adc-49cd-ae98-7ff81de21b3c',
         * ]
         */

        std::string target = "/orders?productId=" + productId;
        std::string response;
        this->privateSession_.isolatedDel(target.c_str(), response);

        rapidjson::Document responseDocument;
        responseDocument.Parse(response.c_str());

        // If is is an object --> failed
        if (responseDocument.IsObject())
        {
            std::cerr << "Failed to get orders | Message: "
                      << responseDocument["message"].GetString() << '\n';
            return;
        }

        if (!responseDocument.IsArray())
        {
            std::cerr << "Invalid response received " << responseDocument.GetString() << '\n';
            return;
        }

        for (auto const &orderIdObj : responseDocument.GetArray())
            output.emplace_back(orderIdObj.GetString());
    }
}
