#include "extras.h"
#include <numeric>
#include <algorithm>
#include <sstream>

Extras g_extras;

std::string Extras::getFrameRenderDebufInfo() {
    std::stringstream ret;
    for (size_t i = 0; i < 10 && i < framerRenderTimes.size(); ++i)
        ret << std::setw(7) << std::setprecision(1) << std::fixed << (framerRenderTimes[i] / 1000.);
    size_t sum = std::accumulate(framerRenderTimes.begin(), framerRenderTimes.end(), 0);
    ret << "\n" << "SUM: " << sum;
    return ret.str();
}