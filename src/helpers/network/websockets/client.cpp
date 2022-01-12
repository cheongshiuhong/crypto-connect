#include "cryptoconnect/helpers/network/websockets/client.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <string>

namespace Network::WebSockets
{
    /* Connector to initialize the context */
    void Client::connect(char const *host, char const *port, char const *target)
    {
        // Resolve the host and port for the IP
        auto const resolverResults = this->resolver_.resolve(host, port);

        // Make the connection on the IP address we got from the lookup
        net::connect(this->ws_.next_layer().next_layer(), resolverResults.begin(), resolverResults.end());

        // Set SNI Hostname
        if (!SSL_set_tlsext_host_name(this->ws_.next_layer().native_handle(), host))
        {
            throw std::runtime_error("Failed to set host name");
        }

        // Perform the SSL handshake
        this->ws_.next_layer().handshake(ssl::stream_base::client);

        // Set a decorator to change the User-Agent of the handshake
        this->ws_.set_option(websocket::stream_base::decorator(
            [](websocket::request_type &req)
            {
                req.set(
                    http::field::user_agent,
                    std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-coro");
            }));

        // Perform the websocket handshake
        this->ws_.handshake(host, target);
    }

    void Client::ping(char const *const message)
    {
        this->ws_.ping(message);
    }

    /* Writes a message to the host (r-value) */
    void Client::write(std::string const &&message)
    {
        // Message becomes l-value in this scope
        this->write(message);
    }

    /* Writes a message to the host (l-value) */
    void Client::write(std::string const &message)
    {
        this->ws_.write(net::buffer(message));
    }

    /* Reads a message from the host */
    void Client::read(std::string &output)
    {
        this->buffer_.clear();
        this->ws_.read(this->buffer_);
        output = boost::beast::buffers_to_string(this->buffer_.cdata());
    }
}
