#ifndef EXTRAS_H
#define EXTRAS_H

#include <string>
#include <map>
#include <framework/core/logger.h>
#include <deque>

constexpr bool default_value = false;

#define DEFINE_OPTION(option, description) m_options[#option] = std::make_pair(description, &(this->option##));

class Extras {
public:
    Extras() {
        DEFINE_OPTION(botDetection, "Bot detection");
        DEFINE_OPTION(newBattleList, "New battle list");
        DEFINE_OPTION(fastOTMLTextRendering, "Fast QTML text rendering");
        DEFINE_OPTION(newMapViewRendering, "New map view (and tile) rendering");
        DEFINE_OPTION(adaptiveRendering, "Adaptive rendering");

        DEFINE_OPTION(OTMLChildIdCache, "OTML child id cache");
        DEFINE_OPTION(fasterAnimations, "Faster animations");    
        DEFINE_OPTION(newWalking, "New walking");    

    }

    bool botDetection = default_value;

    bool fastOTMLTextRendering = default_value;
    bool OTMLChildIdCache = default_value;

    bool newBattleList = default_value;
    bool newMapViewRendering = default_value;
    bool newWalking = default_value;
    bool adaptiveRendering = default_value;
    bool fasterAnimations = default_value;

    int testMode = 0;

    void set(const std::string& key, bool value) {
        auto it = m_options.find(key);
        if (it == m_options.end()) {
            g_logger.fatal(std::string("Invalid extraOptions key:") + key);
            return;
        }
        *(it->second.second) = value;
    }

    bool get(const std::string& key) {
        auto it = m_options.find(key);
        if (it == m_options.end()) {
            g_logger.fatal(std::string("Invalid extraOptions key:") + key);
            return false;
        }
        return *it->second.second;
    }

    std::string getDescription(const std::string& key) {
        auto it = m_options.find(key);
        if (it == m_options.end()) {
            g_logger.fatal(std::string("Invalid extraOptions key:") + key);
            return false;
        }
        return it->second.first;
    }

    std::vector<std::string> getAll() {
        std::vector<std::string> ret;
        for (auto& it : m_options)
            ret.push_back(it.first);
        return ret;
    }

    void setTestMode(int value) {
        g_logger.debug(std::string("Test mode set to: ") + std::to_string(value));
        testMode = value;
    }

    int getTestMode() {
        return testMode;
    }

    void addFrameRenderTime(ticks_t time) {
        framerRenderTimes.push_front(time);
        if (framerRenderTimes.size() > 100)
            framerRenderTimes.pop_back();
    }

    std::string getFrameRenderDebufInfo();

private:
    std::map<std::string, std::pair<std::string, bool*>> m_options;
    std::deque<ticks_t> framerRenderTimes;
};

extern Extras g_extras;

#endif