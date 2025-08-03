#pragma once

#include <string>
#include <memory>
#include <functional>
#include <vector>

class HttpSession;

struct HttpResult {
    HttpResult()
    { }
    HttpResult(const std::string& url, int operationId) :
        url(url), operationId(operationId)
    { }
    std::string url;
    int operationId = 0;
    int status = 0;
    int size = 0;
    int progress = 0; // from 0 to 100
    int redirects = 0; // redirect
    bool connected = false;
    bool finished = false;
    bool canceled = false;
    std::map<std::string, std::string> headers;
    std::vector<uint8_t> body;
    std::string error;
    std::weak_ptr<HttpSession> session;
};

struct HttpRequest {
    HttpRequest()
    { }
    HttpRequest(const std::string& url, int timeout) :
        url(url), timeout(timeout)
    { }
    HttpRequest(const std::string& url, const std::map<std::string, std::string>& headers, int timeout) :
        url(url), headers(headers), timeout(timeout)
    { }
    HttpRequest(const std::string& url, const std::map<std::string, std::string>& headers, const std::string& body, int timeout) :
        url(url), headers(headers), body(body), timeout(timeout)
    { }

    std::string url;
    std::map<std::string, std::string> headers;
    std::string body;
    int timeout = 5;
};


using HttpResult_ptr = std::shared_ptr<HttpResult>;
using HttpRequest_ptr = std::shared_ptr<HttpRequest>;
using HttpResult_cb = std::function<void(HttpResult_ptr)>;
