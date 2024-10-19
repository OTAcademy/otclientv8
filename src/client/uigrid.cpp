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
    m_gridSize(0, 0),
    m_gridWidth(1),
    m_gridColor(Color::white)
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

    if (!m_gridSize.isEmpty() && m_gridSize.isValid()) {
        int numRows = m_rect.height() / m_gridSize.height();
        int numCols = m_rect.width() / m_gridSize.width();

        for (int i = 0; i <= numRows; ++i) {
            int y = (m_rect.topLeft().y + i * m_gridSize.height()) + m_gridWidth;
            std::vector<Point> points = {
                Point(m_rect.topLeft().x, y),
                Point(m_rect.topLeft().x + m_rect.width(), y)
            };
            g_drawQueue->addLine(points, m_gridWidth, m_gridColor);
        }

        for (int i = 0; i <= numCols; ++i) {
            int x = (m_rect.topLeft().x + i * m_gridSize.width()) + m_gridWidth;
            std::vector<Point> points = {
                Point(x, m_rect.topLeft().y),
                Point(x, m_rect.topLeft().y + m_rect.height())
            };
            g_drawQueue->addLine(points, m_gridWidth, m_gridColor);
        }
    }

    drawBorder(m_rect);
    drawIcon(m_rect);
    drawText(m_rect);
}

void UIGrid::onStyleApply(const std::string& styleName, const OTMLNodePtr& styleNode)
{
    UIWidget::onStyleApply(styleName, styleNode);

    for(const OTMLNodePtr& node : styleNode->children()) {
        if (node->tag() == "grid-size")
            m_gridSize = node->value<Size>();
        else if (node->tag() == "grid-width")
            m_gridWidth = node->value<int>();
        else if (node->tag() == "grid-color")
            m_gridColor = node->value<Color>();
    }
}
