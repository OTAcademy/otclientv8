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

#ifndef UIGRID_H
#define UIGRID_H

#include "declarations.h"
#include <framework/ui/uiwidget.h>

class UIGrid : public UIWidget
{
public:
    UIGrid();
    void drawSelf(Fw::DrawPane drawPane);

    void setCellSize(const Size& size);
    Size getCellSize() { return m_cellSize; }

    void setGridWidth(int width);
    int getGridWidth() { return m_gridWidth; }

    void setGridColor(const Color& color) { m_gridColor = color; }
    Color getGridColor() { return m_gridColor; }

protected:
    void onStyleApply(const std::string& styleName, const OTMLNodePtr& styleNode);
    void onGeometryChange(const Rect& oldRect, const Rect& newRect);
    void onLayoutUpdate();
    void onVisibilityChange(bool visible);

    void updateGrid();

private:
    Size m_cellSize;
    int m_gridWidth;
    Color m_gridColor;

    bool m_needsUpdate;
    std::vector<std::vector<Point>> m_points;
};

#endif
