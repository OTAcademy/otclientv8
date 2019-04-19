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

#include "lightview.h"
#include "mapview.h"
#include <framework/graphics/framebuffer.h>
#include <framework/graphics/framebuffermanager.h>
#include <framework/graphics/painter.h>
#include <framework/graphics/image.h>
#include <framework/core/adaptiverenderer.h>

enum {
    MAX_LIGHT_INTENSITY = 8,
    MAX_AMBIENT_LIGHT_INTENSITY = 255
};

LightView::LightView()
{
    m_lightbuffer = g_framebuffers.createFrameBuffer();
    m_tileLightTexture = generateTileLightTexture();
    m_lightTexture = generateLightBubble(0.1f);
    m_blendEquation = Painter::BlendEquation_Add;
    reset();
}

TexturePtr LightView::generateLightBubble(float centerFactor)
{
    int bubbleRadius = 256;
    int centerRadius = bubbleRadius * centerFactor;
    int bubbleDiameter = bubbleRadius * 2;
    ImagePtr lightImage = ImagePtr(new Image(Size(bubbleDiameter, bubbleDiameter)));

    for(int x = 0; x < bubbleDiameter; x++) {
        for(int y = 0; y < bubbleDiameter; y++) {
            float radius = std::sqrt((bubbleRadius - x)*(bubbleRadius - x) + (bubbleRadius - y)*(bubbleRadius - y));
            float intensity = stdext::clamp<float>((bubbleRadius - radius) / (float)(bubbleRadius - centerRadius), 0.0f, 1.0f);

            // light intensity varies inversely with the square of the distance
            intensity = intensity * intensity;
            uint8_t colorByte = intensity * 0xff;

            uint8_t pixel[4] = {colorByte,colorByte,colorByte,0xff};
            lightImage->setPixel(x, y, pixel);
        }
    }

    TexturePtr tex = TexturePtr(new Texture(lightImage, true));
    tex->setSmooth(true);
    return tex;
}

TexturePtr LightView::generateTileLightTexture() {
    ImagePtr tileLight = ImagePtr(new Image(Size(Otc::TILE_PIXELS * 6 / 4, Otc::TILE_PIXELS * 6 / 4)));

    for(int x = 0; x < Otc::TILE_PIXELS * 6 / 4; x++) {
        for(int y = 0; y < Otc::TILE_PIXELS * 6 / 4; y++) {
            uint16_t dist = std::min<uint16_t>(std::min<uint16_t>(x, y),
                                               std::min<uint16_t>(Otc::TILE_PIXELS * 6 / 4 - 1 - x, Otc::TILE_PIXELS * 6 / 4 - 1 - y));
            uint8_t colorByte = 255;
            if (dist < Otc::TILE_PIXELS / 4)
                colorByte = dist * (255 / (Otc::TILE_PIXELS / 4));
            uint8_t pixel[4] = {0,0,0,colorByte};
            tileLight->setPixel(x, y, pixel);
        }
    }

    TexturePtr tex = TexturePtr(new Texture(tileLight, true));
    tex->setSmooth(true);
    return tex;
}

void LightView::reset()
{
    for (auto& it : m_lightMap)
        it.clear();
    for (auto& it : m_creaturesLightMap)
        it.clear();
    for (auto& it : m_hideTiles)
        it.clear();
}

void LightView::resetMapLight()
{
    for (auto& it : m_lightMap)
        it.clear();
    for (auto& it : m_hideTiles)
        it.clear();
}

void LightView::resetCreaturesLight()
{
    for (auto& it : m_creaturesLightMap)
        it.clear();
}

void LightView::setGlobalLight(const Light& light)
{
    m_globalLight = light;
}

void LightView::addLightSource(const Point& center, float scaleFactor, const Light& light, bool fromCreature)
{
    int intensity = std::min<int>(light.intensity, MAX_LIGHT_INTENSITY);
    int radius = intensity * Otc::TILE_PIXELS * scaleFactor;

    Color color = Color::from8bit(light.color);
    float brightness = 0.5f + (intensity/(float)MAX_LIGHT_INTENSITY)*0.5f;

    color.setRed(color.rF() * brightness);
    color.setGreen(color.gF() * brightness);
    color.setBlue(color.bF() * brightness);

    if (fromCreature) {
        if (m_blendEquation == Painter::BlendEquation_Add && m_creaturesLightMap[m_floor].size() > 0) {
            LightSource prevSource = m_creaturesLightMap[m_floor].back();
            if (prevSource.center == center && prevSource.color == color && prevSource.radius == radius)
                return;
        }
    } else {
        if (m_blendEquation == Painter::BlendEquation_Add && m_lightMap[m_floor].size() > 0) {
            LightSource prevSource = m_lightMap[m_floor].back();
            if (prevSource.center == center && prevSource.color == color && prevSource.radius == radius)
                return;
        }
    }

    LightSource source;
    source.center = center;
    source.color = color;
    source.radius = radius;
    source.floor = m_floor;
    if (fromCreature) {
        m_creaturesLightMap[m_floor].push_back(source);
    } else {
        m_lightMap[m_floor].push_back(source);
    }
}

void LightView::hideTile(const Point& pos) {
    m_hideTiles[m_floor].insert(pos);
}


void LightView::drawGlobalLight(const Light& light)
{
    Color color = Color::from8bit(light.color);
    float brightness = light.intensity / (float)MAX_AMBIENT_LIGHT_INTENSITY;
    color.setRed(color.rF() * brightness);
    color.setGreen(color.gF() * brightness);
    color.setBlue(color.bF() * brightness);
    g_painter->setColor(color);
    g_painter->drawFilledRect(Rect(0, 0, m_lightbuffer->getSize()));
}

void LightView::drawLightSource(const Point& center, const Color& color, int radius, float brightness)
{
    // debug draw
    //radius /= 16;

    Rect dest = Rect(center - Point(radius, radius), Size(radius*2,radius*2));
    g_painter->setColor(Color((uint8_t)(color.r() * brightness),(uint8_t)(color.g() * brightness),(uint8_t)(color.b() * brightness),color.a()));
    g_painter->drawTexturedRect(dest, m_lightTexture);
}

void LightView::resize(const Size& size)
{
    m_lightbuffer->resize(size);
}

void LightView::draw(const Rect& dest, const Rect& src)
{
    g_painter->saveAndResetState();
    m_lightbuffer->bind();
    g_painter->clear(Color::black);
    g_painter->setBlendEquation(m_blendEquation);
    g_painter->setCompositionMode(Painter::CompositionMode_Add);
    g_painter->setDepthFunc(Painter::DepthFunc_ALWAYS);

    for (int z = Otc::MAX_Z; z >= 0; --z) {
        g_painter->setCompositionMode(Painter::CompositionMode_Multiply);
        g_painter->setDepthFunc(Painter::DepthFunc_ALWAYS);
        g_painter->setDepth(z);
        g_painter->setColor(Color::black);
        g_painter->setOpacity(m_fading[z]);
        CoordsBuffer cords;
        for (auto& it : m_hideTiles[z])
            cords.addRect(Rect(it, it + Otc::TILE_PIXELS));
        g_painter->drawFillCoords(cords);
        g_painter->setDepthFunc(Painter::DepthFunc_LESS);
        g_painter->resetColor();
        for (auto& it : m_hideTiles[z])
            g_painter->drawTexturedRect(Rect(it - ((Otc::TILE_PIXELS * 1) / 4), it + ((Otc::TILE_PIXELS * 5) / 4)), m_tileLightTexture);
        g_painter->resetOpacity();
        g_painter->setDepthFunc(Painter::DepthFunc_ALWAYS);
        g_painter->setDepth(z * 10);
        g_painter->setCompositionMode(Painter::CompositionMode_Add);
        for (const LightSource& source : m_lightMap[z]) {
            drawLightSource(source.center, source.color, source.radius, m_fading[z]);
        }
        for (const LightSource& source : m_creaturesLightMap[z]) {
            drawLightSource(source.center, source.color, source.radius, m_fading[z]);
        }
    }
    
    drawGlobalLight(m_globalLight);
    m_lightbuffer->release();
    g_painter->setCompositionMode(Painter::CompositionMode_Light);
    m_lightbuffer->draw(dest, src);
    g_painter->restoreSavedState();
}
