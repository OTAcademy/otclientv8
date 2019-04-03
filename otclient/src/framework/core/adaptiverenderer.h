#ifndef ADAPTIVERENDERER_H
#define ADAPTIVERENDERER_H

#include <list>

constexpr int RenderSpeeds = 5;

class AdaptiveRenderer {
public:
    void newFrame();

    void refresh();

    int effetsLimit();

    int creaturesLimit();

    int itemsLimit();

    int mapViewDelay();

    int getLevel() {
        return m_speed;
    }

    int getFps() {
        return m_frames.size() / 5;
    }

    int foregroundUpdateInterval();

    std::string getDebugInfo();

    void setForcedLevel(int value) {
        m_forcedSpeed = value;
    }

private:
    int m_forcedSpeed = -1;
    int m_speed = 0;
    time_t m_update = 0;
    std::list<time_t> m_frames;
};

extern AdaptiveRenderer g_adaptiveRenderer;

#endif