#ifndef UTILS_CRYPTOGRAPHY_H
#define UTILS_CRYPTOGRAPHY_H

#include <string>
#include <sstream>
#include <iomanip>
#include "openssl/hmac.h"
#include "./base64.hpp"

namespace Utils::Crypto
{
    namespace
    {
        typedef EVP_MD const *(*hashFunc_t)();
    }

    /**
     * Raw HMAC to return byte array
     */
    template <hashFunc_t hashFunc>
    inline void hmac(std::string const &secret, std::string const &message,
                     unsigned char *&output, unsigned int &outputSize)
    {
        output = HMAC(hashFunc(), secret.c_str(), secret.size(),
                      reinterpret_cast<unsigned char *>(const_cast<char *>(message.c_str())),
                      message.size(), output, &outputSize);
    }

    /**
     * HMAC to return a Base64 string
     */
    template <hashFunc_t hashFunc>
    inline void hmacBase64(std::string const &secret, std::string const &message,
                           std::string &output)
    {
        unsigned char *result = 0;
        unsigned int resultSize = -1;

        hmac<hashFunc>(secret, message, result, resultSize);

        std::string hmacString(reinterpret_cast<const char *>(result));
        Utils::Base64::encode(hmacString, output);
    }

    /**
     * HMAC to return a hexadecimal string 
     */
    template <hashFunc_t hashFunc>
    inline void hmacHex(std::string const &secret, std::string const &message,
                        std::string &output)
    {
        unsigned char *result = 0;
        unsigned int resultSize = -1;

        hmac<hashFunc>(secret, message, result, resultSize);

        std::ostringstream os;
        for (int i = 0; i < resultSize; i++)
        {
            unsigned int value = static_cast<unsigned>(result[i]);

            // Prepends numbers less than 16 with 0 (e.g. 10 --> '0a' instead of 'a')
            os << std::hex << std::setfill('0') << std::setw(2) << value;
        }

        // Assign the result string to the output
        output = os.str();
    }
}

#endif
