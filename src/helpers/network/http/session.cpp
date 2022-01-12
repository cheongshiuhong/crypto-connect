#include "cryptoconnect/helpers/network/http/session.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <string>

namespace Network::HTTP
{
    /* Constructor */
    Session::Session(char const *host, char const *port)
        : host_(host), port_(port) { this->initSession(); }

    void Session::addHeaders(headers_t const headers)
    {
        this->headers_.insert(headers.begin(), headers.end());
    }

    void Session::addRequestDecorator(requestDecorator_t decorator)
    {
        this->requestDecorators_.push_back(decorator);
    }

    /* HTTP GET Request */
    void Session::get(char const *target, std::string &output)
    {
        // Set up an HTTP POST request message
        request_t req{http::verb::get, target, 11};

        // Make the request
        this->request(req, output);
    }

    /* HTTP POST Request (r-value body) */
    void Session::post(char const *target, std::string &output, std::string const &&body)
    {
        // r-value body becomes l-value in this scope
        this->post(target, output, body);
    }

    /* HTTP POST Request (l-value body) */
    void Session::post(char const *target, std::string &output, std::string const &body)
    {
        // Set up an HTTP POST request message
        request_t req{http::verb::post, target, 11};

        // Set the body and make the request
        req.body() = body;
        this->request(req, output);
    }

    void Session::del(char const *target, std::string &output)
    {
        // Set up an HTTP DELETE request message
        request_t req{http::verb::delete_, target, 11};

        // Make the request
        this->request(req, output);
    }

    /* Private constructor to replicate a session for isolated requests */
    Session::Session(Session *session)
    {
        this->host_ = session->host_;
        this->port_ = session->port_;
        this->headers_ = (session->headers_);
        this->requestDecorators_ = session->requestDecorators_;

        this->initSession();
    }

    /* HTTP GET Request in a separate session - facilitates multithreading */
    void Session::isolatedGet(char const *target, std::string &output)
    {
        // Replicate the session
        Session session(this);
        session.get(target, output);
    }

    /* HTTP POST Request in a separate session - facilitates multithreading */
    void Session::isolatedPost(char const *target, std::string &output)
    {
        // Replicate the session
        Session session(this);
        session.post(target, output);
    }

    void Session::isolatedDel(char const *target, std::string &output)
    {
        // Replicate the session
        Session session(this);
        session.del(target, output);
    }

    /* Called by constuctors to initialize the session */
    void Session::initSession()
    {
        // Resolve the host and port for the IP
        auto const resolverResults = this->resolver_.resolve(this->host_, this->port_);

        // Make the connection on the IP address we got from the lookup
        beast::get_lowest_layer(this->stream_).connect(resolverResults);

        // Set SNI Hostname
        if (!SSL_set_tlsext_host_name(this->stream_.native_handle(), this->host_))
        {
            beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
            throw beast::system_error{ec};
        }

        // Perform the SSL handshake
        this->stream_.handshake(ssl::stream_base::client);
    }

    /* Called by request methods to perform the request */
    void Session::request(request_t &req, std::string &output)
    {
        req.set(http::field::host, this->host_);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Apply our headers gotten from the getters
        for (auto const &decorator : this->requestDecorators_)
            decorator(req);

        // Set our custom headers - overrides any decorator-added headers
        for (auto const &pair : this->headers_)
            req.set(std::get<0>(pair), std::get<1>(pair));

        // Prepare the payload if there are any
        if (req.body().length())
            req.prepare_payload();

        // Send the HTTP request to the remote host
        http::write(this->stream_, req);

        // Declare the containers to hold the response
        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;

        // Receive the HTTP response
        http::read(this->stream_, buffer, res);

        // Write the repsonse body into the output string
        for (auto const seq : res.body().data())
        {
            auto *cbuf = boost::asio::buffer_cast<char const *>(seq);
            output.append(cbuf, boost::asio::buffer_size(seq));
        }
    }
}
