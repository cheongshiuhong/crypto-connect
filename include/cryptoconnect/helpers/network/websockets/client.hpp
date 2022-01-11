#ifndef NETWORK_WEBSOCKETS_CLIENT_H
#define NETWORK_WEBSOCKETS_CLIENT_H

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <string>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace Network::WebSockets
{
    using request_t = websocket::request_type;

    class Client
    {
    private:
        net::io_context ioc_;
        ssl::context ctx_{ssl::context::tlsv13_client};
        tcp::resolver resolver_{this->ioc_};
        websocket::stream<beast::ssl_stream<tcp::socket>> ws_{this->ioc_, this->ctx_};
        beast::flat_buffer buffer_;
        const char *host_;

    public:
        /* Connector to initialize the context */
        void connect(const char *host, const char *port, const char *target = "/");

        /* Pings the host */
        void ping(char const *const message);

        /* Writes a message to the host (l-value) */
        void write(const std::string &message);

        /* Writes a message to the host (r-value) */
        void write(const std::string &&message);

        /* Reads a message from the host */
        void read(std::string &output);
    };
}

#endif
