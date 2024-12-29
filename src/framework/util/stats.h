// tfs stats system made by BBarwik

#ifndef OTCLIENT_STATS_H
#define OTCLIENT_STATS_H

#include <list>
#include <atomic>
#include <mutex>
#include <chrono>
#include <string>
#include <unordered_map>
#include <set>

// NOT THREAD SAFE

enum StatsTypes{
    STATS_FIRST = 0,
    STATS_GENERAL = STATS_FIRST,
    STATS_MAIN,
    STATS_RENDER,
    STATS_DISPATCHER,
    STATS_LUA,
    STATS_LUACALLBACK,
    STATS_PACKETS,
    STATS_LAST = STATS_PACKETS
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

class UIWidget;

class Stats {
public:
    void add(int type, Stat* stats);

    std::string get(int type, int limit, bool pretty);
    void clear(int type);

    void clearAll();

    std::string getSlow(int type, int limit, unsigned int minTime, bool pretty);
    void clearSlow(int type);

    int types() { return STATS_LAST + 1; }

    int64_t getSleepTime() {
        return m_sleepTime;
    }
    void resetSleepTime() {
        m_sleepTime = 0;
    }

    int64_t m_sleepTime = 0;

    void addWidget(UIWidget* widget);
    void removeWidget(UIWidget* widget);
    std::string getWidgetsInfo(int limit, bool pretty);

    inline void addTexture() { createdTextures += 1; }
    inline void removeTexture() { destroyedTextures += 1; }

    inline void addThing() { createdThings += 1; }
    inline void removeThing() { destroyedThings += 1; }

    inline void addCreature() { createdCreatures += 1; }
    inline void removeCreature() { destroyedCreatures += 1; }

private:
    struct {
        StatsMap data;
        StatsList slow;
        int64_t start = 0;
    } stats[STATS_LAST + 1];

    std::set<UIWidget*> widgets;
    int createdWidgets = 0;
    int destroyedWidgets = 0;
    int createdTextures = 0;
    int destroyedTextures = 0;
    int createdThings = 0;
    int destroyedThings = 0;
    int createdCreatures = 0;
    int destroyedCreatures = 0;
    std::mutex m_mutex;
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
    int m_type;
    Stat* m_stat;

protected:
    uint64_t m_minusTime = 0;
    std::chrono::high_resolution_clock::time_point m_timePoint;
};

#endif
