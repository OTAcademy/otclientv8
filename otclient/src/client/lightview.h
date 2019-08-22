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

#ifndef LIGHTVIEW_H
#define LIGHTVIEW_H

#include "declarations.h"
#include <framework/graphics/declarations.h>
#include <framework/graphics/painter.h>
#include <set>
#include "thingtype.h"

class LightView : public LuaObject
{
public:
    LightView();

    void reset();
    void resetMapLight();
    void resetCreaturesLight();

    void setFloor(int floor) {
        m_floor = stdext::clamp<int>(floor, 0, Otc::MAX_Z);
    }

    void setGlobalLight(const Light& light) {
        m_globalLight = light;
    }

    void addLightSource(const Point& center, const Light& light, bool fromCreature = false);
    void addGround(const Point& ground);

    void resize(const Size& size);
    void draw(const Rect& dest, const Rect& src);

private:
    void drawGlobalLight(const Light& light);
    TexturePtr generateLightBubble(float centerFactor);

    Light m_globalLight;
    TexturePtr m_lightTexture;

    FrameBufferPtr m_lightbuffer;

    LightBuffer m_mapLight;
    LightBuffer m_creaturesLight;

    Point prevCenter;
    Light prevLight;

    std::map<PointF, float> m_ground; 
    bool m_updateDepth = false;
    int m_floor = 0;
};

#endif
