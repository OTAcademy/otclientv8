/*
 * Copyright (c) 2010-2017 OTClient <https://github.com/edubart/otclient>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "uigrid.h"
#include <framework/otml/otml.h>
#include <framework/graphics/graphics.h>
#include <framework/graphics/texturemanager.h>
#include <client/spritemanager.h>

UIGrid::UIGrid() :
    m_cellSize(0, 0),
    m_gridWidth(1),
    m_gridColor(Color::white),
    m_needsUpdate(false)
{ }

void UIGrid::drawSelf(Fw::DrawPane drawPane)
{
    if(drawPane != Fw::ForegroundPane)
        return;

    // draw style components in order
    if(m_backgroundColor.aF() > Fw::MIN_ALPHA) {
        Rect backgroundDestRect = m_rect;
        backgroundDestRect.expand(-m_borderWidth.top, -m_borderWidth.right, -m_borderWidth.bottom, -m_borderWidth.left);
        drawBackground(m_rect);
    }

    drawImage(m_rect);

    if (m_needsUpdate) {
        updateGrid();
    }

    for (const auto& points : m_points) {
        g_drawQueue->addLine(points, m_gridWidth, m_gridColor);
    }

    drawBorder(m_rect);
    drawIcon(m_rect);
    drawText(m_rect);
}

void UIGrid::onStyleApply(const std::string& styleName, const OTMLNodePtr& styleNode)
{
    UIWidget::onStyleApply(styleName, styleNode);

    for(const OTMLNodePtr& node : styleNode->children()) {
        if (node->tag() == "cell-size")
            setCellSize(node->value<Size>());
        else if (node->tag() == "grid-width")
            setGridWidth(node->value<int>());
        else if (node->tag() == "grid-color")
            setGridColor(node->value<Color>());
    }
}

void UIGrid::onGeometryChange(const Rect& oldRect, const Rect& newRect)
{
    UIWidget::onGeometryChange(oldRect, newRect);
    m_needsUpdate = true;
}

void UIGrid::onLayoutUpdate()
{
    UIWidget::onLayoutUpdate();
    m_needsUpdate = true;
}

void UIGrid::onVisibilityChange(bool visible)
{
    UIWidget::onVisibilityChange(visible);
    m_needsUpdate = visible;
}

void UIGrid::setCellSize(const Size& size)
{
    m_cellSize = size;
    m_needsUpdate = true;
}

void UIGrid::setGridWidth(int width)
{
    m_gridWidth = width;
    m_needsUpdate = true;
}

void UIGrid::updateGrid()
{
    m_points.clear();

    if (!m_rect.isEmpty() && m_rect.isValid()) {
        if (!m_cellSize.isEmpty() && m_cellSize.isValid()) {
            auto dest = getPaddingRect();
            int numRows = dest.height() / m_cellSize.height();
            int numCols = dest.width() / m_cellSize.width();

            for (int i = 1; i <= numRows; ++i) {
                int y = (dest.topLeft().y + i * m_cellSize.height());
                std::vector<Point> points = {
                    Point(dest.topLeft().x, y),
                    Point(dest.topLeft().x + dest.width(), y)
                };
                m_points.push_back(points);
            }

            for (int i = 1; i <= numCols; ++i) {
                int x = (dest.topLeft().x + i * m_cellSize.width());
                std::vector<Point> points = {
                    Point(x, dest.topLeft().y),
                    Point(x, dest.topLeft().y + dest.height())
                };
                m_points.push_back(points);
            }

            m_needsUpdate = false;
        }
    }
}