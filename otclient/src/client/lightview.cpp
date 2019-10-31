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
#include "spritemanager.h"
#include <framework/graphics/framebuffer.h>
#include <framework/graphics/framebuffermanager.h>
#include <framework/graphics/painter.h>
#include <framework/graphics/image.h>
#include <framework/graphics/graphics.h>
#include <framework/core/adaptiverenderer.h>

enum {
    MAX_LIGHT_INTENSITY = 8,
    MAX_AMBIENT_LIGHT_INTENSITY = 255,
    LIGHT_SCALING = 32,
    DEPTH_PER_FLOOR = 500
};

LightView::LightView()
{
    m_lightbuffer = g_framebuffers.createFrameBuffer(true);
    m_lightbuffer->setSmooth(true);
    m_lightTexture = generateLightBubble(0.1f);
    m_mapLight.setLightTexture(m_lightTexture);
    m_creaturesLight.setLightTexture(m_lightTexture);
    reset();
}

TexturePtr LightView::generateLightBubble(float centerFactor)
{
    int bubbleRadius = 32;
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

    TexturePtr tex = TexturePtr(new Texture(lightImage, false));
    tex->setSmooth(true);
    return tex;
}

void LightView::reset()
{
    resetMapLight();
    resetCreaturesLight();
}

void LightView::resetMapLight()
{
    m_mapLight.size = 0;
    m_ground.clear();
    m_updateDepth = true;
    prevCenter = Point(0, 0);
}

void LightView::resetCreaturesLight()
{
    m_creaturesLight.size = 0;
    prevCenter = Point(0, 0);
}

void LightView::addLightSource(const Point& center, const Light& light, bool fromCreature)
{
    if (!m_mapLight.texture || m_mapLight.size >= 1024 || m_creaturesLight.size >= 1024)
        return;
    if (prevCenter == center && light.color == prevLight.color && light.intensity == prevLight.intensity)
        return;
    prevLight = light;
    prevCenter = center;

    int intensity = std::min<int>(light.intensity, MAX_LIGHT_INTENSITY);

    Color color = Color::from8bit(light.color);
    float brightness = 0.5f + (intensity/(float)MAX_LIGHT_INTENSITY)*0.5f;

    LightBuffer& buffer = fromCreature ? m_creaturesLight : m_mapLight;

    for (int i = 0; i < 6; ++i) {
        buffer.colorBuffer[buffer.size * 24 + i * 4] = color.rF() * brightness;
        buffer.colorBuffer[buffer.size * 24 + i * 4 + 1] = color.gF() * brightness;
        buffer.colorBuffer[buffer.size * 24 + i * 4 + 2] = color.bF() * brightness;
        buffer.colorBuffer[buffer.size * 24 + i * 4 + 3] = 1.0f;
        buffer.depthBuffer[buffer.size * 6] = m_floor * 500;
    }    
    float radius = intensity * Otc::TILE_PIXELS;
    RectF rect(center.x - radius, center.y - radius, radius * 2, radius * 2);
    buffer.addRect(&buffer.destCoords[buffer.size * 12], rect * (1.0f / LIGHT_SCALING));
    buffer.size += 1;
}

void LightView::addGround(const Point& ground)
{
    float gdepth = m_floor * DEPTH_PER_FLOOR + DEPTH_PER_FLOOR / 2;
    auto& depth = m_ground.emplace(PointF(((float)ground.x) / LIGHT_SCALING, ((float)ground.y) / LIGHT_SCALING), gdepth).first->second;
    if (depth > gdepth)
        depth = gdepth;
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

void LightView::resize(const Size& size)
{
    m_lightbuffer->resize(size / LIGHT_SCALING);
}

void LightView::draw(const Rect& dest, const Rect& src)
{
    g_painter->saveAndResetState();

    // DRAW LIGHTS
    m_lightbuffer->bind();

    g_painter->setAlphaWriting(true);
    if (g_graphics.canUseDepth()) {
        g_painter->setCompositionMode(Painter::CompositionMode_Replace);

        if (m_updateDepth) {
            m_updateDepth = false;
            g_painter->setDepthFunc(Painter::DepthFunc_LESS);
            g_painter->clear(Color::black);

            g_painter->drawLightDepth(m_ground, LIGHT_SCALING);
        }

        g_painter->setDepthFunc(Painter::DepthFunc_LESS_READ);
    }

    // global light
    Color color = Color::from8bit(m_globalLight.color);
    float brightness = m_globalLight.intensity / (float)MAX_AMBIENT_LIGHT_INTENSITY;
    color.setRed(color.rF() * brightness);
    color.setGreen(color.gF() * brightness);
    color.setBlue(color.bF() * brightness);
    if (g_graphics.canUseDepth()) {
        g_painter->setDepthFunc(Painter::DepthFunc_LEQUAL_READ);
        g_painter->setColor(color);
        g_painter->setDepth(0);
        g_painter->drawFilledRect(Rect(0, 0, m_lightbuffer->getSize()));
        g_painter->resetColor();
    } else {
        g_painter->clear(color);
    }

    // lights
    g_painter->setCompositionMode(Painter::CompositionMode_Add);
    g_painter->drawLights(m_mapLight);
    g_painter->drawLights(m_creaturesLight);

    m_lightbuffer->release();

    // DRAW LIGHT
    g_painter->setCompositionMode(Painter::CompositionMode_Light);
    float vertical = (float)dest.height() / (float)src.height();
    float horizontal = (float)dest.width() / (float)src.width();
    Point mt = Point(src.topLeft().x * horizontal, src.topLeft().y * vertical);
    Point mb = (m_lightbuffer->getSize().toPoint() * LIGHT_SCALING) - Point(src.bottomRight().x, src.bottomRight().y);
    mb = Point(mb.x * horizontal, mb.y * vertical);
    m_lightbuffer->draw(Rect(dest.topLeft() - mt, dest.bottomRight() + mb));

    g_painter->restoreSavedState();
}
