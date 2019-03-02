// tfs stats system made by BBarwik

#ifndef TFS_STATS_H
#define TFS_STATS_H

#include <forward_list>
#include <atomic>
#include <mutex>
#include <chrono>
#include <map>
#include <cassert>

struct Stat {
    Stat(uint64_t _executionTime, const std::string& _description, const std::string& _extraDescription) :
            executionTime(_executionTime), description(_description), extraDescription(_extraDescription) {};
    uint64_t executionTime = 0;
    std::string description;
    std::string extraDescription;
};

struct statsData {
    statsData(uint32_t _calls, uint64_t _executionTime, const std::string& _extraInfo) :
            calls(_calls), executionTime(_executionTime), extraInfo(_extraInfo) {}
    uint32_t calls = 0;
    uint64_t executionTime = 0;
    std::string extraInfo;
};

using statsMap = std::map<std::string, statsData>;

class Stats {
public:
	/*
    void threadMain();
    void shutdown() {
        setState(THREAD_STATE_TERMINATED);
    }

    void addDispatcherTask(int index, Task* task);
    void addLuaStats(Stat* stats);
    void addSqlStats(Stat* stats); */
    void addSpecialStats(Stat* stats);
    /* std::atomic<uint64_t>& dispatcherWaitTime(int index) {
        return dispatchers[index].waitTime;
    }  */
    std::string getSpecial();

private:
//    void parseDispatchersQueue(std::vector<std::forward_list < Task * >> queues);
//    void parseLuaQueue(std::forward_list <Stat*>& queue);
//    void parseSqlQueue(std::forward_list <Stat*>& queue);
    void parseSpecialQueue(std::forward_list <Stat*>& queue);
//    void writeSlowInfo(const std::string& file, uint64_t executionTime, const std::string& description, const std::string& extraDescription);
//    void writeStats(const std::string& file, const statsMap& stats, const std::string& extraInfo = "");

    uint64_t startTime = 0;
    std::mutex statsLock;
    struct {
        std::forward_list <Stat*> queue;
        statsMap stats;
        int64_t lastDump;
    } lua, sql, special;
};

extern Stats g_stats;

class AutoStat {
public:
    AutoStat(const std::string& description, const std::string& extraDescription = "") :
            stat(new Stat(0, description, extraDescription)), time_point(std::chrono::high_resolution_clock::now()) {}

    ~AutoStat() {
        stat->executionTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - time_point).count();
        stat->executionTime -= minusTime;
        g_stats.addSpecialStats(stat);
    }

private:
    Stat* stat;

protected:
    uint64_t minusTime = 0;
    std::chrono::high_resolution_clock::time_point time_point;
};

// NOT THREAD SAFE
class AutoStatRecursive : public AutoStat {
public:
    AutoStatRecursive(const std::string& description, const std::string& extraDescription = "") : AutoStat(description, extraDescription) {
        parent = activeStat;
        activeStat = this;
    }
    ~AutoStatRecursive() {
        activeStat = parent;
        if(activeStat) 
            activeStat->minusTime += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - time_point).count();
    }

private:
    static AutoStatRecursive* activeStat;
    AutoStatRecursive* parent;
};

#endif