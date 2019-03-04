#include <framework/core/logger.h>
#include <framework/stdext/format.h>
#include <framework/util/extras.h>

#include "adaptiverenderer.h"

AdaptiveRenderer g_adaptiveRenderer;

void AdaptiveRenderer::updateLastRenderTime(size_t microseconds) {
    if (!g_extras.adaptiveRendering) {
        speed = 0;
        return;
    }

    avg = (microseconds + avg * 59) / 60;

    if (speed == RenderSpeeds - 1) {
        // nothing to be done
    } else if (microseconds > 30000) {
        speed = min(speed + 1, RenderSpeeds - 1);
    } else if (avg > 12000) { // >12ms for frame
        speed = min(speed + 1, RenderSpeeds - 1);
    }

    if (avg < 8000 && speed != 0) { // <8ms for frame
        avg += 4000;
        speed -= 1;
    }
}

int AdaptiveRenderer::effetsLimit() const {
    static int limits[RenderSpeeds] = { 20, 10, 5, 3, 1 };
    return limits[speed];
}

int AdaptiveRenderer::creaturesLimit() const {
    static int limits[RenderSpeeds] = { 20, 10, 7, 4, 2 };
    return limits[speed];
}

int AdaptiveRenderer::itemsLimit() const {
    static int limits[RenderSpeeds] = { 20, 10, 7, 4, 2 };
    return limits[speed];
}

bool AdaptiveRenderer::ignoreLight() const {
    return speed >= 3;
}