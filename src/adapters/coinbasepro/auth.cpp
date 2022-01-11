#include "adapters/coinbasepro/auth.hpp"

#include "helpers/network/http/session.hpp"
#include "helpers/utils/base64.hpp"
#include "helpers/utils/cryptography.hpp"

#include <yaml/yaml.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <cstdlib>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace CryptoConnect::CoinbasePro
{
    Auth::Auth()
    {
        Yaml::Node config;
        Yaml::Parse(config, "config.yaml");

        this->apiKey_ = config["coinbasepro"][COINBASEPRO_CONFIG_KEY]["apiKey"].As<std::string>();
        this->passPhrase_ = config["coinbasepro"][COINBASEPRO_CONFIG_KEY]["passPhrase"].As<std::string>();

        // Secret key needs to be decoded from Base64
        std::string base64SecretKey = config["coinbasepro"][COINBASEPRO_CONFIG_KEY]["secretKey"].As<std::string>();
        Utils::Base64::decode(base64SecretKey, this->secretKey_);
    }

    /* Decorator to add headers into the request*/
    void Auth::addAuthHeaders(Network::HTTP::request_t &req)
    {
        // Get the timestamp now in string (CoinbasePro uses seconds)
        std::string timestampString;
        this->getTimestampString(timestampString);

        // Construct the auth message
        std::stringstream authMessage;
        authMessage << timestampString << req.method() << req.target() << req.body();

        // Get the HMAC string
        std::string hmacBase64String;
        this->getSignature(authMessage.str(), hmacBase64String);

        // Set the headers for the CoinbasePro request
        req.set("Content-Type", "application/json");
        req.set("CB-ACCESS-KEY", this->apiKey_);
        req.set("CB-ACCESS-TIMESTAMP", timestampString);
        req.set("CB-ACCESS-SIGN", hmacBase64String);
        req.set("CB-ACCESS-PASSPHRASE", this->passPhrase_);
    }

    void Auth::getTimestampString(std::string &output)
    {
        output = std::to_string(
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count());
    }

    void Auth::getSignature(std::string const &message, std::string &output)
    {
        Utils::Crypto::hmacBase64<EVP_sha256>(this->secretKey_, message, output);

        // Strip away any invalid characters
        boost::replace_all(output, " ", "");
        boost::replace_all(output, "\n", "");
    }

}
