#include <framework/luaengine/luainterface.h>
#include <framework/util/crypt.h>
#include <framework/util/stats.h>

#include "http.h"
#include "session.h"

Http g_http;

void Http::init() {
    m_working = true;
}

void Http::terminate() {
    if (!m_working)
        return;
    m_working = false;
    m_ios.stop();
}

void Http::poll() {
    AutoStat s(STATS_MAIN, "PollHTTP");
    m_ios.reset();
    m_ios.poll();
}

int Http::get(const std::string& url, int timeout) {
    if (!timeout) // lua is not working with default values
        timeout = 5;
    auto result = std::make_shared<HttpResult>();
    result->url = url;
    result->operationId = m_operationId;
    m_operations[m_operationId] = result;
    auto session = std::make_shared<HttpSession>(m_ios, url, timeout, result, [&] (HttpResult_ptr result) {   
        if (!result->finished) {
            g_lua.callGlobalField("g_http", "onGetProgress", result->operationId, result->url, result->progress);
            return;
        }
        g_lua.callGlobalField("g_http", "onGet", result->operationId, result->url, result->error, std::string(result->response.begin(), result->response.end()));
    });
    session->start();
    return m_operationId++;
}

int Http::post(const std::string& url, const std::string& data, int timeout) {
    if (!timeout) // lua is not working with default values
        timeout = 5;
    if (data.empty()) {
        g_logger.error(stdext::format("Invalid post request for %s, empty data, use get instead", url));
        return -1;
    }
    auto result = std::make_shared<HttpResult>();
    result->url = url;
    result->operationId = m_operationId;
    result->postData = data;
    m_operations[m_operationId] = result;
    auto session = std::make_shared<HttpSession>(m_ios, url, timeout, result, [&] (HttpResult_ptr result) {     
        if (!result->finished) {
            g_lua.callGlobalField("g_http", "onPostProgress", result->operationId, result->url, result->progress);
            return;
        }
        g_lua.callGlobalField("g_http", "onPost", result->operationId, result->url, result->error, std::string(result->response.begin(), result->response.end()));
    });
    session->start();
    return m_operationId++;
}

int Http::download(const std::string& url, std::string path, int timeout) {
    if (!timeout) // lua is not working with default values
        timeout = 5;
    auto result = std::make_shared<HttpResult>();
    result->url = url;
    result->operationId = m_operationId;
    m_operations[m_operationId] = result;
    auto session = std::make_shared<HttpSession>(m_ios, url, timeout, result, [&, path] (HttpResult_ptr result) {
        m_speed = ((result->size) * 10) / (1 + stdext::micros() - m_lastSpeedUpdate);
        m_lastSpeedUpdate = stdext::micros();

        if (!result->finished) {
            g_lua.callGlobalField("g_http", "onDownloadProgress", result->operationId, result->url, result->progress, m_speed);
            return;
        }
        if (result->error.empty()) {
            if (!path.empty() && path[0] == '/')
                m_downloads[path.substr(1)] = result;
            else
                m_downloads[path] = result;
        }
        std::string checksum = g_crypt.md5Encode(std::string(result->response.begin(), result->response.end()), false);
        g_lua.callGlobalField("g_http", "onDownload", result->operationId, result->url, result->error, path, checksum);
    });
    session->start();
    return m_operationId++;
}

bool Http::cancel(int id) {
    auto it = m_operations.find(id);
    if (it == m_operations.end())
        return false;
    if (it->second->canceled)
        return false;
    it->second->canceled =true;
    return true;
}

int Http::getProgress(int id) {
    auto it = m_operations.find(id);
    if (it == m_operations.end())
        return -1;
    if (it->second->finished)
        return 100;
    return it->second->progress;
}
