#ifndef CRYPTOCONNECT_COINBASEPRO_AUTH_H
#define CRYPTOCONNECT_COINBASEPRO_AUTH_H

#if IS_SANDBOX
#define COINBASEPRO_CONFIG_KEY "sandbox"
#else
#define COINBASEPRO_CONFIG_KEY "live"
#endif

#include "cryptoconnect/helpers/network/http/session.hpp"

#include <string>

namespace CryptoConnect::CoinbasePro
{
    class Auth
    {
    private:
        std::string secretKey_;

    public:
        std::string apiKey_;
        std::string passPhrase_;

        Auth();

        void addAuthHeaders(Network::HTTP::request_t &req);
        void getTimestampString(std::string &output);
        void getSignature(std::string const &message, std::string &output);
    };
}

#endif
