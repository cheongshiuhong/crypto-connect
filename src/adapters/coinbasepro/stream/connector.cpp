#include "adapters/coinbasepro/stream/connector.hpp"

#include "helpers/network/websockets/client.hpp"
#include "structs/universe.hpp"
#include "adapters/coinbasepro/auth.hpp"
#include "adapters/coinbasepro/stream/handler.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <thread>
#include <sstream>
#include <string>

namespace Adapters::CoinbasePro::Stream
{
    /* Constructor */
    Connector::Connector(Auth *auth) : auth_(auth) {}

    /* Connects to the socket */
    void Connector::connect()
    {
        this->wsClient_.connect(COINBASEPRO_WS_ENDPOINT, "443");

        // Invoke keepalive pinging
        std::thread keepAliveThread(
            [this]
            { this->keepAlive(); });

        keepAliveThread.detach();
    }

    /* Loops and calls the callback on receiving a message */
    void Connector::streamForever(Handler &handler)
    {
        std::string message;

        while (1)
        {
            message.clear();
            this->wsClient_.read(message);
            // message = R"({"type": "l2update", "product_id": "BTC-USD", "changes": [["buy", "1.23", "400.32"]], "time": "2022-01-01T10:00:00.123456"})";
            handler.onMessage(message);
        }
    }

    /* Subscribes to products */
    void Connector::subscribeProducts(Universe::Universe const &universe)
    {
        std::string message;
        this->makeSubscriptionMessage("subscribe", universe, message);
        this->wsClient_.write(message);
    }

    /* Unsubscribes to products */
    void Connector::unsubscribeProducts(Universe::Universe const &universe)
    {
        // Guard clause for when called with empty universe
        if (!universe.size())
            return;

        std::string message;
        this->makeSubscriptionMessage("unsubscribe", universe, message);
        this->wsClient_.write(message);
    }

    /* Constructs the subscription message from the type and the unvierse */
    void Connector::makeSubscriptionMessage(
        std::string const type,
        Universe::Universe const &universe,
        std::string &output)
    {
        boost::property_tree::ptree messageTree;
        boost::property_tree::ptree productIdsArray;
        boost::property_tree::ptree channelsArray;

        // Subscription type
        messageTree.put("type", type);

        // Product IDs
        for (auto const &productId : universe)
        {
            boost::property_tree::ptree productChild;
            productChild.put("", productId);
            productIdsArray.push_back(std::make_pair("", productChild));
        }
        messageTree.add_child("product_ids", productIdsArray);

        // Channels (level2 -> ticks, ticker -> trades, user -> order statuses & transactions)
        for (auto const &channel : {"level2", "ticker", "user"})
        {
            boost::property_tree::ptree channelChild;
            channelChild.put("", channel);
            channelsArray.push_back(std::make_pair("", channelChild));
        }
        messageTree.add_child("channels", channelsArray);

        // Get auth details
        std::string timestampString, authMessage, signedAuthMessage;
        this->auth_->getTimestampString(timestampString);
        authMessage = timestampString + "GET" + "/users/self/verify";
        this->auth_->getSignature(authMessage, signedAuthMessage);

        // Add auth details
        boost::property_tree::ptree timestampProperty;
        timestampProperty.put("", timestampString);
        messageTree.add_child("timestamp", timestampProperty);

        boost::property_tree::ptree apiKeyProperty;
        apiKeyProperty.put("", this->auth_->apiKey_);
        messageTree.add_child("key", apiKeyProperty);

        boost::property_tree::ptree signatureProperty;
        signatureProperty.put("", signedAuthMessage);
        messageTree.add_child("signature", signatureProperty);

        boost::property_tree::ptree passPhraseProperty;
        passPhraseProperty.put("", this->auth_->passPhrase_);
        messageTree.add_child("passphrase", passPhraseProperty);

        std::stringstream ss;
        boost::property_tree::json_parser::write_json(ss, messageTree);

        output = ss.str();
    }

    void Connector::keepAlive()
    {
        while (1)
        {
            std::this_thread::sleep_for(std::chrono::seconds(30));
            this->wsClient_.ping("keepalive");
        }
    }
}
