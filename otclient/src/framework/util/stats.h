// tfs stats system made by BBarwik

#ifndef TFS_STATS_H
#define TFS_STATS_H

#include <list>
#include <atomic>
#include <mutex>
#include <chrono>
#include <unordered_map>
#include <cassert>

// NOT THREAD SAFE

enum StatsTypes{
    STATS_FIRST = 0,
    STATS_GENERAL = STATS_FIRST,
    STATS_MAIN,
    STATS_RENDER,
    STATS_DISPATCHER,
    STATS_LUA,
    STATS_LUACALLBACK,
    STATS_LAST = STATS_LUACALLBACK
};

struct Stat {
    Stat(uint64_t _executionTime, const std::string& _description, const std::string& _extraDescription) :
            executionTime(_executionTime), description(_description), extraDescription(_extraDescription) {};
    uint64_t executionTime = 0;
    std::string description;
    std::string extraDescription;
};

struct StatsData {
    StatsData(uint32_t _calls, uint64_t _executionTime, const std::string& _extraInfo) :
            calls(_calls), executionTime(_executionTime), extraInfo(_extraInfo) {}
    uint32_t calls = 0;
    uint64_t executionTime = 0;
    std::string extraInfo;
};

using StatsMap = std::unordered_map<std::string, StatsData>;
using StatsList = std::list<Stat*>;

class Stats {
public:
    void add(int type, Stat* stats);

    std::string get(int type, int limit, bool pretty);
    void clear(int type);

    std::string getSlow(int type, int limit, int minTime, bool pretty);
    void clearSlow(int type);

    int types() { return STATS_LAST + 1; }

private:
    struct {
        StatsMap data;
        StatsList slow;
        int64_t start = 0;
    } stats[STATS_LAST + 1];
};

extern Stats g_stats;

class AutoStat {
public:
    AutoStat(int type, const std::string& description, const std::string& extraDescription = "") :
            m_type(type), m_stat(new Stat(0, description, extraDescription)), m_timePoint(std::chrono::high_resolution_clock::now()) {}

    ~AutoStat() {
        m_stat->executionTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - m_timePoint).count();
        m_stat->executionTime -= m_minusTime;
        g_stats.add(m_type, m_stat);
    }

    AutoStat(const AutoStat&) = delete;
    AutoStat & operator=(const AutoStat&) = delete;

private:
    Stat* m_stat;
    int m_type;

protected:
    uint64_t m_minusTime = 0;
    std::chrono::high_resolution_clock::time_point m_timePoint;
};

class AutoStatRecursive : public AutoStat {
public:
    AutoStatRecursive(const std::string& description, const std::string& extraDescription = "") : AutoStat(STATS_GENERAL, description, extraDescription) {
        parent = activeStat;
        activeStat = this;
    }

    ~AutoStatRecursive() {
        activeStat = parent;
        if(activeStat) 
            activeStat->m_minusTime += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - m_timePoint).count();
    }

    AutoStatRecursive(const AutoStatRecursive&) = delete;
    AutoStatRecursive & operator=(const AutoStatRecursive&) = delete;

private:
    static AutoStatRecursive* activeStat; /* = nullptr */
    AutoStatRecursive* parent;
};

#endif