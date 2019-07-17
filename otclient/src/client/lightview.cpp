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
    MAX_AMBIENT_LIGHT_INTENSITY = 255
};

LightView::LightView()
{
    m_lightbuffer = g_framebuffers.createFrameBuffer();
    m_lightbuffer2 = g_framebuffers.createFrameBuffer();
    m_lightbuffer3 = g_framebuffers.createFrameBuffer();
    m_lightbuffer->setSmooth(true);
    m_lightbuffer2->setSmooth(false);
    m_lightbuffer3->setSmooth(true);
    m_lightTexture = generateLightBubble(0.1f);
    reset();
}

TexturePtr LightView::generateLightBubble(float centerFactor)
{
    int bubbleRadius = 64;
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

void LightView::reset()
{
    resetMapLight();
    resetCreaturesLight();
}

void LightView::resetMapLight()
{
    m_lightMap.clear();
    m_updateDepth = true;
}

void LightView::resetCreaturesLight()
{
    m_creaturesLightMap.clear();
}

void LightView::setGlobalLight(const Light& light, int lightScaling)
{
    m_globalLight = light;
    if (lightScaling != m_scaling) {
        std::swap(m_scaling, lightScaling);
        resize((m_lightbuffer->getSize() * lightScaling));
    }
}

void LightView::addLightSource(const Point& center, const Light& light, bool fromCreature)
{
    int intensity = std::min<int>(light.intensity, MAX_LIGHT_INTENSITY);
    int radius = intensity * g_sprites.spriteSize();

    Color color = Color::from8bit(light.color);
    float brightness = 0.5f + (intensity/(float)MAX_LIGHT_INTENSITY)*0.5f;

    color.setRed(color.rF() * brightness);
    color.setGreen(color.gF() * brightness);
    color.setBlue(color.bF() * brightness);

    LightSource source;
    source.color = color;
    source.radius = radius;
    source.depth = m_depth;
    if (fromCreature) {
        auto& newLight = m_creaturesLightMap.emplace(center, source).first->second;
        if (newLight.radius < source.radius) {
            newLight.radius = source.radius;
            newLight.color = source.color;
        }
        if (newLight.depth > source.depth) {
            newLight.depth = source.depth;
        }
    } else {
        auto& newLight = m_lightMap.emplace(center, source).first->second;
        if (newLight.radius < source.radius) {
            newLight.radius = source.radius;
            newLight.color = source.color;
        }
        if (newLight.depth > source.depth) {
            newLight.depth = source.depth;
        }
    }
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
    m_lightbuffer->resize(size / m_scaling);
    m_lightbuffer2->resize(size / m_scaling);
    m_lightbuffer3->resize(size / 32);
}

void LightView::draw(const Rect& dest, const Rect& src, TexturePtr depthTexture)
{
    g_painter->saveAndResetState();

    // DEPTH SCALING
    if (m_updateDepth) {
        m_updateDepth = false;
        m_lightbuffer2->bind();
        g_painter->setAlphaWriting(true);
        g_painter->setCompositionMode(Painter::CompositionMode_Replace);
        if (depthTexture)
            g_painter->drawLightDepthTexture(Rect(0, 0, m_lightbuffer2->getSize()), depthTexture, Rect(0, 0, depthTexture->getSize()));
        else
            g_painter->clear(Color::white);
        m_lightbuffer2->release();
    }
    
    // DRAW LIGHTS
    m_lightbuffer->bind();
    g_painter->setAlphaWriting(true);
    g_painter->setCompositionMode(Painter::CompositionMode_Add);
    
    // global light
    Color color = Color::from8bit(m_globalLight.color);
    float brightness = m_globalLight.intensity / (float)MAX_AMBIENT_LIGHT_INTENSITY;
    color.setRed(color.rF() * brightness);
    color.setGreen(color.gF() * brightness);
    color.setBlue(color.bF() * brightness);
    g_painter->clear(color);

    // other lights
    g_painter->drawLights(m_lightMap, m_lightTexture, m_lightbuffer2->getTexture(), m_scaling);
    g_painter->drawLights(m_creaturesLightMap, m_lightTexture, m_lightbuffer2->getTexture(), m_scaling);

    m_lightbuffer->release();
    
    // SMOOTH LIGHT
    m_lightbuffer3->bind();
    g_painter->setAlphaWriting(true);
    g_painter->setCompositionMode(Painter::CompositionMode_Replace);
    g_painter->clear(Color::black);
    m_lightbuffer->draw(Rect(0, 0, m_lightbuffer3->getSize()), Rect(0, 0, m_lightbuffer->getSize()));
    m_lightbuffer3->release();

    // DRAW LIGHT
    g_painter->setCompositionMode(Painter::CompositionMode_Light);
    float vertical = (float)dest.height() / (float)src.height();
    float horizontal = (float)dest.width() / (float)src.width();
    Point mt = Point(src.topLeft().x * horizontal, src.topLeft().y * vertical);
    Point mb = (m_lightbuffer->getSize().toPoint() * m_scaling) - Point(src.bottomRight().x, src.bottomRight().y);
    mb = Point(mb.x * horizontal, mb.y * vertical);
    m_lightbuffer3->draw(Rect(dest.topLeft() - mt, dest.bottomRight() + mb));

    g_painter->restoreSavedState();
}
