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

struct LightSource {
    Color color;
    Point center;
    int radius;
    uint8 floor;
};

class LightView : public LuaObject
{
public:
    LightView();

    void reset();
    void resetMapLight();
    void resetCreaturesLight();
    void setGlobalLight(const Light& light);
    void addLightSource(const Point& center, float scaleFactor, const Light& light, bool fromCreature = false);
    void hideTile(const Point& pos);
    void setFloor(uint8_t floor, float fading) { m_floor = stdext::clamp<uint8_t>(floor, 0, Otc::MAX_Z); m_fading[m_floor] = fading; }

    void resize(const Size& size);
    void draw(const Rect& dest, const Rect& src);

    void setBlendEquation(Painter::BlendEquation blendEquation) { m_blendEquation = blendEquation; }

private:
    void drawGlobalLight(const Light& light);
    void drawLightSource(const Point& center, const Color& color, int radius, float brightness);
    TexturePtr generateLightBubble(float centerFactor);

    TexturePtr generateTileLightTexture();

    Painter::BlendEquation m_blendEquation;
    TexturePtr m_lightTexture;
    TexturePtr m_tileLightTexture;
    FrameBufferPtr m_lightbuffer;
    Light m_globalLight;
    uint8_t m_floor = 0;
    std::vector<LightSource> m_lightMap[Otc::MAX_Z + 1];
    std::vector<LightSource> m_creaturesLightMap[Otc::MAX_Z + 1];
    std::set<Point> m_hideTiles[Otc::MAX_Z + 1];
    float m_fading[Otc::MAX_Z + 1] = { 1 };
};

#endif
