#include "stats.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>
#include <framework/stdext/time.h>

Stats g_stats;

AutoStatRecursive* AutoStatRecursive::activeStat = nullptr;

void Stats::add(int type, Stat* stat) {
    if (type > STATS_LAST)
        return;


    auto it = stats[type].data.emplace(stat->description, StatsData(0, 0, stat->extraDescription)).first;
    it->second.calls += 1;
    it->second.executionTime += stat->executionTime;

    if (stat->executionTime > 1000) {
        if (stats[type].slow.size() > 10000) {
            delete stats[type].slow.front();
            stats[type].slow.pop_front();
        }
        stats[type].slow.push_back(stat);
    } else
        delete stat;
}

std::string Stats::get(int type, int limit, bool pretty) {
    if (type > STATS_LAST)
        return "";

    std::multimap<uint64_t, StatsMap::const_iterator> sorted_stats;
    
    uint64_t total_time = 0;
    uint64_t time_from_start = (stdext::micros() - stats[type].start);

    for (auto it = stats[type].data.cbegin(); it != stats[type].data.cend(); ++it) {
        sorted_stats.emplace(it->second.executionTime, it);
        total_time += it->second.executionTime;
    }

    std::stringstream ret;

    int i = 0;
    if (pretty)
        ret << "Function" << std::setw(42) << "Calls" << std::setw(10) << "Time (ms)" << std::setw(10) << "Time (%)" << std::setw(10) << "Cpu (%)" << "\n";
    else
        ret << "Stats|" << type << "|" << limit << "|" << stdext::micros() << "|" << total_time << "|" << time_from_start << "\n";

    for (auto it = sorted_stats.rbegin(); it != sorted_stats.rend(); ++it) {
        if (i++ > limit)
            break;
        if (pretty) {
            std::string name = it->second->first.substr(0, 45);
            ret << name << std::setw(50 - name.size()) << it->second->second.calls << std::setw(10) << (it->second->second.executionTime / 1000)
                << std::setw(10) << ((it->second->second.executionTime * 100) / (total_time)) << std::setw(10) << ((it->second->second.executionTime * 100) / (time_from_start)) << "\n";
        } else {
            ret << it->second->first << "|" << it->second->second.calls << "|" << it->second->second.executionTime << "\n";
        }
    }

    return ret.str();
}

void Stats::clear(int type) {
    if (type > STATS_LAST)
        return;
    stats[type].start = stdext::micros();
    stats[type].data.clear();
}

void Stats::clearAll() {
    for (int i = 0; i <= STATS_LAST; ++i) {
        stats[i].data.clear();
        stats[i].slow.clear();
    }
}

std::string Stats::getSlow(int type, int limit, int minTime, bool pretty) {
    if (type > STATS_LAST)
        return "";

    std::stringstream ret;

    int i = 0;
    if (pretty)
        ret << "Function" << std::setw(42) << "Time (ms)" << std::setw(20) << "Extra description" << "\n";
    else
        ret << "Slow|" << type << "|" << limit << "|" << minTime << "|" << stdext::micros() << "\n";

    minTime *= 1000;

    for (auto it = stats[type].slow.rbegin(); it != stats[type].slow.rend(); ++it) {
        if ((*it)->executionTime < (minTime))
            continue;
        if (i++ > limit)
            break;
        if (pretty) {
            std::string name = (*it)->description.substr(0, 45);
            ret << name << std::setw(50 - name.size()) << (*it)->executionTime / 1000 << std::setw(20) << (*it)->extraDescription << "\n";
        } else {
            ret << (*it)->description << "|" << (*it)->executionTime << "|" << (*it)->extraDescription << "\n";
        }
    }

    return ret.str();
}

void Stats::clearSlow(int type) {
    if (type > STATS_LAST)
        return;
    for (auto& stat : stats[type].slow)
        delete stat;
    stats[type].slow.clear();
}
