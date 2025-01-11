#ifndef UIGRAPH_H
#define UIGRAPH_H

#include "declarations.h"
#include <framework/ui/uiwidget.h>

struct GraphData {
    std::vector<Point> points;
    std::deque<int> values;
    Point infoLine[2];
    Rect originalInfoRect;
    Rect infoRect;
    Rect infoRectBg;
    Rect infoRectIcon;
    Rect infoIndicator;
    Rect infoIndicatorBg;
    std::string infoValue;
    std::string infoText;
    Color infoLineColor;
    Color infoTextBg;
    Color lineColor;
    int width;
    int infoIndex;
    bool visible;
};

class UIGraph : public UIWidget {
public:
    UIGraph();

    void drawSelf(Fw::DrawPane drawPane);

    void clear();
    size_t createGraph();
    void addValue(size_t index, int value, bool ignoreSmallValues = false);

    void setCapacity(int capacity) {
        m_capacity = capacity;
        m_needsUpdate = true;
    }
    void setTitle(const std::string& title) { m_title = title; }
    void setShowLabels(bool value) { m_showLabes = value; }
    void setShowInfo(bool value) { m_showInfo = value; }

    void setLineWidth(size_t index, int width);
    void setLineColor(size_t index, const Color& color);
    void setInfoText(size_t index, const std::string& text);
    void setInfoLineColor(size_t index, const Color& color);
    void setTextBackground(size_t index, const Color& color);
    void setGraphVisible(size_t index, bool visible);
    size_t getGraphsCount() { return m_graphs.size(); }

protected:
    void onStyleApply(const std::string& styleName, const OTMLNodePtr& styleNode);
    void onGeometryChange(const Rect& oldRect, const Rect& newRect);
    void onLayoutUpdate();
    void onVisibilityChange(bool visible);

    void cacheGraphs();
    void updateGraph(GraphData& graph, bool& updated);
    void updateInfoBoxes();

private:
    // cache
    bool m_needsUpdate;
    std::string m_minValue;
    std::string m_maxValue;
    std::string m_lastValue;

    std::string m_title;
    bool m_showLabes;
    bool m_showInfo;

    size_t m_capacity;
    size_t m_ignores;

    std::vector<GraphData> m_graphs;
};

#endif