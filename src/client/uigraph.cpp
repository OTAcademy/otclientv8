
#include "uigraph.h"
#include <framework/graphics/drawqueue.h>
#include <framework/graphics/textrender.h>
#include <framework/graphics/fontmanager.h>
#include <framework/core/eventdispatcher.h>

UIGraph::UIGraph() : m_needsUpdate(false) {}

void UIGraph::drawSelf(Fw::DrawPane drawPane)
{
    if (drawPane != Fw::ForegroundPane)
        return;

    if (m_backgroundColor.aF() > Fw::MIN_ALPHA) {
        Rect backgroundDestRect = m_rect;
        backgroundDestRect.expand(-m_borderWidth.top, -m_borderWidth.right, -m_borderWidth.bottom, -m_borderWidth.left);
        drawBackground(m_rect);
    }

    drawImage(m_rect);

    if (m_needsUpdate) {
        updateGraph();
    }

    if (!m_points.empty()) {
        g_drawQueue->addLine(m_points, m_width, m_color);

        Rect dest = getPaddingRect();
        if (!m_title.empty())
            g_drawQueue->addText(m_font, m_title, dest, Fw::AlignTopCenter);
        if (m_showLabes) {
            g_drawQueue->addText(m_font, m_lastValue, dest, Fw::AlignTopRight);
            g_drawQueue->addText(m_font, m_maxValue, dest, Fw::AlignTopLeft);
            g_drawQueue->addText(m_font, m_minValue, dest, Fw::AlignBottomLeft);
        }
    }

    drawBorder(m_rect);
    drawIcon(m_rect);
    drawText(m_rect);
}

void UIGraph::clear()
{
    m_values.clear();
    m_points.clear();
}

void UIGraph::addValue(int value, bool ignoreSmallValues)
{
    if (ignoreSmallValues) {
        if (!m_values.empty() && m_values.back() <= 2 && value <= 2 && ++m_ignores <= 10)
            return;
        m_ignores = 0;
    }
    m_values.push_back(value);
    while (m_values.size() > m_capacity)
        m_values.pop_front();

    m_needsUpdate = true;
}

void UIGraph::onStyleApply(const std::string& styleName, const OTMLNodePtr& styleNode)
{
    UIWidget::onStyleApply(styleName, styleNode);

    for (const OTMLNodePtr& node : styleNode->children()) {
        if (node->tag() == "line-width")
            setLineWidth(node->value<int>());
        else if (node->tag() == "capacity")
            setCapacity(node->value<int>());
        else if (node->tag() == "title")
            setTitle(node->value<std::string>());
        else if (node->tag() == "show-labels")
            setShowLabels(node->value<bool>());
    }
}

void UIGraph::onGeometryChange(const Rect& oldRect, const Rect& newRect)
{
    UIWidget::onGeometryChange(oldRect, newRect);
    m_needsUpdate = true;
}

void UIGraph::onLayoutUpdate()
{
    UIWidget::onLayoutUpdate();
    m_needsUpdate = true;
}

void UIGraph::onVisibilityChange(bool visible)
{
    UIWidget::onVisibilityChange(visible);
    m_needsUpdate = visible;
}

void UIGraph::updateGraph()
{
    if (!m_needsUpdate)
        return;

    m_points.clear();

    if (!m_rect.isEmpty() && m_rect.isValid()) {
        if (!m_values.empty()) {
            Rect dest = getPaddingRect();

            float offsetX = dest.left();
            float offsetY = dest.top();
            size_t elements = std::min<size_t>(m_values.size(), dest.width() / (m_width * 2) - 1);
            size_t start = m_values.size() - elements;
            int minVal = 0xFFFFFF, maxVal = -0xFFFFFF;
            for (size_t i = start; i < m_values.size(); ++i) {
                if (minVal > m_values[i])
                    minVal = m_values[i];
                if (maxVal < m_values[i])
                    maxVal = m_values[i];
            }

            // round
            maxVal = (1 + maxVal / 10) * 10;
            minVal = (minVal / 10) * 10;
            float step = (float)(dest.height()) / std::max<float>(1, (maxVal - minVal));
            for (size_t i = start, j = 0; i < m_values.size(); ++i) {
                m_points.push_back(Point(offsetX + j * m_width, offsetY + 1 + (maxVal - m_values[i]) * step));
                j += 2;
            }

            m_minValue = std::to_string(minVal);
            m_maxValue = std::to_string(maxVal);
            m_lastValue = std::to_string(m_values.back());
        }

        m_needsUpdate = false;
    }
}
