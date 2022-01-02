#ifndef NETWORK_HTTP_SESSION_H
#define NETWORK_HTTP_SESSION_H

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http;   // from <boost/beast/http.hpp>
namespace net = boost::asio;    // from <boost/asio.hpp>
namespace ssl = net::ssl;       // from <boost/asio/ssl.hpp>
using tcp = net::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace Network::HTTP
{
    using request_t = http::request<http::string_body>;
    using headers_t = std::unordered_map<char const *, char const *>;
    using requestDecorator_t = std::function<void(request_t &)>;
    using requestDecorators_t = std::vector<requestDecorator_t>;

    class Session
    {
    private:
        net::io_context ioc_;
        ssl::context ctx_{ssl::context::tlsv13_client};
        tcp::resolver resolver_{this->ioc_};
        beast::ssl_stream<beast::tcp_stream> stream_{this->ioc_, this->ctx_};
        beast::flat_buffer buffer_;

        /* Tracked for creating separate sessions */
        char const *host_;
        char const *port_;

        /* Custom headers and decorators - direct headers override decorator-added headers */
        headers_t headers_;
        requestDecorators_t requestDecorators_;

    public:
        /* Constructor */
        Session(char const *host, char const *port);

        /* Add headers into subsequent requests */
        void addHeaders(const headers_t headers);

        /* Add a decorator to apply onto subsequent requests*/
        void addRequestDecorator(requestDecorator_t decorator);

        /**
         * Request Methods
         * 
         * Any query parameters to be included in target
         * e.g. api.app.com + query {"field1": 123}
         *  --> api.app.com?field1=123
         */

        /* HTTP GET Request */
        void get(char const *target, std::string &output);

        /* HTTP POST Request (r-value body)*/
        void post(char const *target, std::string &output, const std::string &&body);

        /* HTTP POST Request (l-value body) */
        void post(char const *target, std::string &output, const std::string &body = "");

        /* HTTP Delete Request */
        void del(char const *target, std::string &output);

        /**
         * Isolated Request Methods
         * 
         * Creates a separate session instance
         * to facilitate multithreading
         * albeit slightly slower.
         */

        /* HTTP GET Request in a separate session - facilitates multithreading */
        void isolatedGet(char const *target, std::string &output);

        /* HTTP POST Request in a separate session - facilitates multithreading */
        void isolatedPost(char const *target, std::string &output);

        /* HTTP DELETE Request in a separate session - facilitates multithreading */
        void isolatedDel(char const *target, std::string &output);

    private:
        /* Private constructor to replicate a session for isolated requests */
        Session(Session *session);

        /* Called by constuctors to initialize the session */
        void initSession();

        /* Called by request methods to perform the request */
        void request(http::request<http::string_body> &req, std::string &output);
    };
}

#endif