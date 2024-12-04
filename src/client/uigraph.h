#ifndef UIGRAPH_H
#define UIGRAPH_H

#include "declarations.h"
#include <framework/ui/uiwidget.h>

class UIGraph : public UIWidget {
public:
    UIGraph();

    void drawSelf(Fw::DrawPane drawPane);

    void clear();
    void addValue(int value, bool ignoreSmallValues = false);
    void setLineWidth(int width)
    {
        m_width = width;
        m_needsUpdate = true;
    }
    void setCapacity(int capacity)
    {
        m_capacity = capacity;
        m_needsUpdate = true;
    }
    void setTitle(const std::string& title)
    {
        m_title = title;
    }
    void setShowLabels(bool value)
    {
        m_showLabes = value;
    }

protected:
    void onStyleApply(const std::string& styleName, const OTMLNodePtr& styleNode);
    void onGeometryChange(const Rect& oldRect, const Rect& newRect);
    void onLayoutUpdate();
    void onVisibilityChange(bool visible);

    void updateGraph();

private:
    bool m_needsUpdate;
    std::vector<Point> m_points;
    std::string m_minValue;
    std::string m_maxValue;
    std::string m_lastValue;

    std::string m_title;
    size_t m_capacity = 100;
    size_t m_ignores = 0;
    int m_width = 1;
    bool m_showLabes = true;
    std::deque<int> m_values;
};

#endif