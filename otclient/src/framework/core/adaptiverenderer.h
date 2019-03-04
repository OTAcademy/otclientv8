#ifndef ADAPTIVERENDERER_H
#define ADAPTIVERENDERER_H

constexpr int RenderSpeeds = 5;

class AdaptiveRenderer {
public:
    void updateLastRenderTime(size_t microseconds);
    int effetsLimit() const;

    int creaturesLimit() const;

    int itemsLimit() const;

    bool ignoreLight() const;

    int getLevel() const {
        return speed;
    }

    int getAvg() const {
        return avg;
    }

private:
    int speed = 0;
    int avg = 0; 
};

extern AdaptiveRenderer g_adaptiveRenderer;

#endif