// hidden code of lightview

#include "lightview.h"
#include <framework/graphics/painter.h>

void LightView::draw() // render thread
{
    // TODO: optimize in the future for big areas
    static std::vector<uint8_t> buffer;
    if (buffer.size() < 4u * m_mapSize.area())
        buffer.resize(m_mapSize.area() * 4);

    for (int x = 0; x < m_mapSize.width(); ++x) {
        for (int y = 0; y < m_mapSize.height(); ++y) {
            Point pos(x * Otc::TILE_PIXELS + Otc::TILE_PIXELS / 2, y * Otc::TILE_PIXELS + Otc::TILE_PIXELS / 2);
            int index = (y * m_mapSize.width() + x);
            int colorIndex = index * 4;
            buffer[colorIndex] = m_globalLight.r();
            buffer[colorIndex + 1] = m_globalLight.g();
            buffer[colorIndex + 2] = m_globalLight.b();
            buffer[colorIndex + 3] = 255; // alpha channel
            for (size_t i = m_tiles[index].start; i < m_lights.size(); ++i) {
                Light& light = m_lights[i];
                float distance = std::sqrt((pos.x - light.pos.x) * (pos.x - light.pos.x) +
                                           (pos.y - light.pos.y) * (pos.y - light.pos.y));
                distance /= Otc::TILE_PIXELS;
                float intensity = (-distance + light.intensity) * 0.2f;
                if (intensity < 0.01f) continue;
                if (intensity > 1.0f) intensity = 1.0f;
                Color lightColor = Color::from8bit(light.color) * intensity;
                buffer[colorIndex] = std::max<int>(buffer[colorIndex], lightColor.r());
                buffer[colorIndex + 1] = std::max<int>(buffer[colorIndex + 1], lightColor.g());
                buffer[colorIndex + 2] = std::max<int>(buffer[colorIndex + 2], lightColor.b());
            }
        }
    }

    m_lightTexture->update();
    glBindTexture(GL_TEXTURE_2D, m_lightTexture->getId());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_mapSize.width(), m_mapSize.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());

    Point offset = m_src.topLeft();
    Size size = m_src.size();
    CoordsBuffer coords;
    coords.addRect(RectF(m_dest.left(), m_dest.top(), m_dest.width(), m_dest.height()),
                   RectF((float)offset.x / Otc::TILE_PIXELS, (float)offset.y / Otc::TILE_PIXELS,
                         (float)size.width() / Otc::TILE_PIXELS, (float)size.height() / Otc::TILE_PIXELS));

    g_painter->resetColor();
    g_painter->setCompositionMode(Painter::CompositionMode_Multiply);
    g_painter->drawTextureCoords(coords, m_lightTexture);
    g_painter->resetCompositionMode();
}
