#ifndef  HTTP_H
#define HTTP_H

#include <thread>
#include <boost/asio.hpp>
#include <map>
#include "result.h"

class Http {
public:
    Http() {}

    void init();
    void terminate();
    void poll();

    int get(const std::string& url, int timeout = 5);
    int download(const std::string& url, const std::string& path, int timeout = 5);

    int getProgress(int id);

    const std::map<std::string, HttpResult_ptr>& downloads() {
        return m_downloads;
    }
    void clearDownloads() {
        m_downloads.clear();
    }

private:
    bool m_working = false;
    int m_operationId = 1;
    boost::asio::io_context m_ios;
    std::map<int, HttpResult_ptr> m_operations;
    std::map<std::string, HttpResult_ptr> m_downloads;
};

extern Http g_http;

#endif // ! HTTP_H
