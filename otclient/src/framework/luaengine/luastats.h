#ifndef LUASTATS_H
#define LUASTATS_H

#include <map>
#include <string>

class LuaStats {
public:
    LuaStats();
    void add(const std::string& function, uint64_t executionTime);
    void addFromCallback(const std::string& function, uint64_t executionTime);
    std::string getAsString(int limit = 30);
    std::string getCallbackAsString(int limit = 30);
    std::map<std::string, std::pair<uint64_t, uint64_t>> getAndClear() {        
        auto ret = std::move(m_stats);
        clear();
        return std::move(ret);
    }
    
    void clear();

private:
    using StatsMap = std::map<std::string, std::pair<uint64_t, uint64_t>>;
    StatsMap m_stats;
    StatsMap m_callbackStats;
    uint64_t m_start;

};

extern LuaStats g_luaStats;


#endif
