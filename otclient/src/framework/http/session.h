#pragma once

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/dynamic_body.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/http/parser.hpp>

#include <iostream>
#include <string>
#include <memory>
#include <functional>
#include <future>

#include "result.h"

class HttpSession : public std::enable_shared_from_this<HttpSession>
{
public:

    HttpSession(boost::asio::io_service& service, const std::string& url, int timeout, HttpResult_ptr result, HttpResult_cb callback) :
        m_url(url), m_socket(service), m_resolver(service), m_callback(callback), m_result(result), m_timer(service), m_timeout(timeout)
    {
        BOOST_ASSERT(m_callback);
        BOOST_ASSERT(m_result);
    };

    void start();
    
private:
    std::string m_url;
    boost::asio::ip::tcp::socket m_socket;
    std::unique_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>> m_ssl;
    boost::asio::ip::tcp::resolver m_resolver;
    boost::asio::steady_timer m_timer;
    int m_timeout;
    boost::beast::flat_buffer m_streambuf{ 512 * 1024 * 1024 }; // limited to 512MB
    boost::beast::http::request<boost::beast::http::string_body> m_request;
    boost::beast::http::response_parser<boost::beast::http::dynamic_body> m_response;
    HttpResult_ptr m_result;
    HttpResult_cb m_callback;

    void on_resolve(const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::iterator iterator);
    void on_connect(const boost::system::error_code& ec);
    void on_request_sent(const boost::system::error_code& ec);
    void on_read_header(const boost::system::error_code & ec, size_t bytes_transferred);
    void on_read(const boost::system::error_code& ec, size_t bytes_transferred);
    void close();
    void onTimeout(const boost::system::error_code& error);
    void onError(const std::string& error, const std::string& details = "");
};