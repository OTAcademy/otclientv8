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
    if (speed == RenderSpeeds - 1) {
        // nothing to be done
    } else if (microseconds > 30000) {
        speed = min(speed + 2, RenderSpeeds - 1);
        g_logger.debug(stdext::format("Set render speed to %i", speed));
    } else if (microseconds > 15000) {
        speed = min(speed + 1, RenderSpeeds - 1);
        g_logger.debug(stdext::format("Set render speed to %i", speed));
    }

    avg = (microseconds + avg * 59) / 60;
    if (avg < 8000 && speed != 0) {
        avg += 5000;
        speed -= 1;
        g_logger.debug(stdext::format("Set render speed to %i", speed));
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