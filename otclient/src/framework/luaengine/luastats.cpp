#include <sstream>
#include <iomanip>
#include <framework/stdext/time.h>

#include "luastats.h"

LuaStats g_luaStats;

LuaStats::LuaStats() {
    m_start = stdext::micros();
}

void LuaStats::clear() {
    m_start = stdext::micros();
    m_stats.clear();
    m_callbackStats.clear();
}

void LuaStats::add(const std::string& function, uint64_t executionTime) {
    auto it = m_stats.emplace(function, std::make_pair(0, 0));
    it.first->second.first += 1;
    it.first->second.second += executionTime;
}

void LuaStats::addFromCallback(const std::string& function, uint64_t executionTime) {
    auto it = m_callbackStats.emplace(function, std::make_pair(0, 0));
    it.first->second.first += 1;
    it.first->second.second += executionTime;
}

std::string LuaStats::getAsString(int limit) {
    std::multimap<uint64_t, StatsMap::const_iterator> sorted_stats;
    uint64_t total_time = 0;
    uint64_t time_from_start = stdext::micros() - m_start;
    for (StatsMap::const_iterator it = m_stats.cbegin(); it != m_stats.cend(); ++it) {
        sorted_stats.emplace(it->second.second, it);
        total_time += it->second.second;
    }
    std::stringstream ret;
    int i = 0;
    ret << "Function" << std::setw(32) << "Calls" << std::setw(10) << "Time (ms)" << std::setw(10) << "Time (%)" << std::setw(10) << "Cpu (%)" << "\n";
    for (auto it = sorted_stats.rbegin(); it != sorted_stats.rend(); ++it) {
        if (i++ > limit)
            break;
        std::string function_name = it->second->first.substr(0, 30);
        ret << function_name << std::setw(40 - function_name.size()) << it->second->second.first << std::setw(10) << (it->second->second.second / 1000)
            << std::setw(10) << ((it->second->second.second * 100) / total_time) << std::setw(10) << ((it->second->second.second * 100) / time_from_start) << "\n";
    }
    return ret.str();
}

std::string LuaStats::getCallbackAsString(int limit) {
    std::multimap<uint64_t, StatsMap::const_iterator> sorted_stats;
    uint64_t total_time = 0;
    uint64_t time_from_start = stdext::micros() - m_start;
    for (StatsMap::const_iterator it = m_callbackStats.cbegin(); it != m_callbackStats.cend(); ++it) {
        sorted_stats.emplace(it->second.second, it);
        total_time += it->second.second;
    }
    std::stringstream ret;
    int i = 0;
    ret << "Function" << std::setw(52) << "Calls" << std::setw(10) << "Time (ms)" << std::setw(10) << "Time (%)" << std::setw(10) << "Cpu (%)" << "\n";
    for (auto it = sorted_stats.rbegin(); it != sorted_stats.rend(); ++it) {
        if (i++ > limit)
            break;
        std::string function_name = it->second->first.substr(0, 30);
        ret << function_name << std::setw(60 - function_name.size()) << it->second->second.first << std::setw(10) << (it->second->second.second / 1000)
            << std::setw(10) << ((it->second->second.second * 100) / total_time) << std::setw(10) << ((it->second->second.second * 100) / time_from_start) << "\n";
    }
    return ret.str();
}
