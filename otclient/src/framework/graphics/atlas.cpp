#include "atlas.h"
#include "framebuffermanager.h"
#include "painter.h"
#include "graphics.h"
#include <framework/core/clock.h>

Atlas g_atlas;

void Atlas::init() {
    m_size = std::min<size_t>(4096, g_graphics.getMaxTextureSize());
    g_logger.info(stdext::format("[Atlas] Texture size is: %ix%i (max: %ix%i)", m_size, m_size, g_graphics.getMaxTextureSize(), g_graphics.getMaxTextureSize()));

    for (int l = 0; l < 2; ++l) {
        m_atlas[l] = g_framebuffers.createFrameBuffer(false);
        m_atlas[l]->resize(Size(m_size, m_size));
    }
    reset();
}

void Atlas::reset() {
    for (int l = 0; l < 2; ++l) {
        for (int j = 0; j < 4; ++j)
            m_locations[l][j].clear();
        m_outfits[l].clear();
        m_textures[l].clear();

        for (int x = 0; x < m_size; x += 256) {
            for (int y = 0; y < m_size; y += 256) {
                m_locations[l][3].push_back(Point(x, y));
            }
        }
        // create 32x32 white square
        findSpace(l, 0);
        assert(!m_locations[l][0].empty());
        m_locations[l][0].pop_front();
        m_atlas[l]->bind();
        g_painter->setAlphaWriting(true);
        g_painter->clearRect(Color::white, Rect(Point(0, 0), Size(32, 32)));
        m_atlas[l]->release();
    }
}


void Atlas::terminate() {
    for(auto& it : m_atlas)
        it = nullptr;
    for(auto& it : m_textures)
        it.clear();
}

void Atlas::update(int location, DrawQueue& queue) {
    if (location >= 2 || !m_atlas[location])
        return;

    static const size_t sizes[4] = { 32, 64, 128, 256 };
    auto& textures = m_textures[location];

    bool bound = false;
    for (auto& item : queue.items()) {
        auto& tex = item.texture;
        if (!tex) { // solid color
            item.src = Rect(0, 0, 8, 8);
            continue;
        }
        size_t size = std::max<size_t>(tex->getSize().width(), tex->getSize().height());
        if (size > 256) {
            item.cached = false;
            continue;
        }
        auto it = textures.find(tex);
        if (it != textures.end()) {
            item.src = Rect(item.src.topLeft() + it->second.first, item.src.size());
            it->second.second = g_clock.millis();
            continue;
        }

        int index = (int)(std::log2(size / 32));
        if (m_locations[location][index].empty() && !findSpace(location, index)) {
            item.cached = false;
            continue;
        }
        if (!bound) {
            bound = true;
            m_atlas[location]->bind();
            g_painter->setAlphaWriting(true);
            g_painter->setDepthFunc(Painter::DepthFunc_ALWAYS);
            g_painter->setCompositionMode(Painter::CompositionMode_Replace);
        }
        auto loc = m_locations[location][index].front();
        m_locations[location][index].pop_front();
        g_painter->clearRect(Color::alpha, Rect(loc, tex->getSize()));
        g_painter->resetColor();
        g_painter->drawTexturedRect(Rect(loc, tex->getSize()), tex, Rect(0, 0, tex->getSize()));
        textures[tex] = std::make_pair(loc, g_clock.millis());
        item.src = Rect(item.src.topLeft() + loc, item.src.size());
    }
    
    for (auto& item : queue.outfits()) {
        auto& key = item.key;
        size_t size = 64;

        auto it = m_outfits[location].find(key);
        if (it != m_outfits[location].end()) {
            queue.setOpacity(item.opacity);
            for (auto& square : item.squares)
                queue.add(square);
            queue.add(Rect(item.dest - Point(32, 32), Size(64, 64)), nullptr, Rect(it->second.first, Size(64, 64)), Color::white, item.depth);
            it->second.second = g_clock.millis();
            continue;
        }

        int index = (int)(std::log2(size / 32));
        if (m_locations[location][index].empty() && !findSpace(location, index)) {
            item.cached = false;
            continue;
        }

        if (!bound) {
            bound = true;
            m_atlas[location]->bind();
            g_painter->setAlphaWriting(true);
            g_painter->setDepthFunc(Painter::DepthFunc_ALWAYS);
            g_painter->setCompositionMode(Painter::CompositionMode_Replace);
        }
        auto loc = m_locations[location][index].front();
        m_locations[location][index].pop_front();
        g_painter->clearRect(Color::alpha, Rect(loc, Size(64,64)));

        drawOutfit(loc, item);

        m_outfits[location][key] = std::make_pair(loc, g_clock.millis());

        queue.setOpacity(item.opacity);
        for (auto& square : item.squares)
            queue.add(square);
        queue.add(Rect(item.dest - Point(32, 32), Size(64, 64)), nullptr, Rect(loc, Size(64, 64)), Color::white, item.depth);
    }
    if (bound) {
        m_atlas[location]->release();
    }
    queue.setAtlas(m_atlas[location]->getTexture());
}

void Atlas::drawOutfit(const Point& location, const DrawQueueOutfit& outfit) {
    g_painter->setCompositionMode(Painter::CompositionMode_Normal);
    g_painter->resetColor();
    if(outfit.mount.texture)
        g_painter->drawTexturedRect(outfit.mount.dest + location, outfit.mount.texture, outfit.mount.src);
    for (auto& texture : outfit.textures) {
        g_painter->setCompositionMode(Painter::CompositionMode_Normal);
        if(texture.texture.texture)  // todo fix naming
            g_painter->drawTexturedRect(texture.texture.dest + location, texture.texture.texture, texture.texture.src);
        for (auto& layer : texture.layers) {
            g_painter->setCompositionMode(Painter::CompositionMode_Multiply);
            if (layer.texture) {
                g_painter->setColor(layer.color);
                g_painter->drawTexturedRect(layer.dest + location, layer.texture, layer.src);
            }
        }
        g_painter->resetColor();
    }
    g_painter->setCompositionMode(Painter::CompositionMode_Replace);
}


bool Atlas::findSpace(int location, int index) {
    static const size_t sizes[4] = { 32, 64, 128, 256 };
    if (location < 2 && index < 3) {
        if (m_locations[location][index + 1].size() == 0 && !findSpace(location, index + 1)) {
            return false;
        }
        auto pos = m_locations[location][index + 1].front();
        m_locations[location][index + 1].pop_front();
        m_locations[location][index].push_back(pos);
        m_locations[location][index].push_back(Point(pos.x, pos.y + sizes[index]));
        m_locations[location][index].push_back(Point(pos.x + sizes[index], pos.y));
        m_locations[location][index].push_back(Point(pos.x + sizes[index], pos.y + sizes[index]));
        return true;
    }
    return false;
}

void Atlas::clean(bool fastClean) {
    if (!m_atlas[0])
        return;

    for (int l = 0; l < 2; ++l) {
        size_t count = 0;
        for (int i = 0; i < 4; ++i) {
            count += m_locations[l][i].size() * (i + 1) * (i + 1);
        }
        if ((count < 100 && fastClean) || count < 1000) {
            int limit = fastClean ? 10 : 100;
            for (auto it = m_textures[l].begin(); it != m_textures[l].end(); ) {
                if (it->second.second + (fastClean ? 2000 : 60000) < g_clock.millis()) {
                    size_t size = std::max<size_t>(it->first->getWidth(), it->first->getHeight());
                    assert(size <= 256);
                    int index = (int)(std::log2(size / 32));
                    m_locations[l][index].push_back(it->second.first);
                    it = m_textures[l].erase(it);
                    if (--limit <= 0)
                        break;
                } else {
                    ++it;
                }
            }
            limit = fastClean ? 10 : 100;
            for (auto it = m_outfits[l].begin(); it != m_outfits[l].end(); ) {
                if (it->second.second + (fastClean ? 2000 : 60000) < g_clock.millis()) {
                    size_t size = 64;
                    assert(size <= 256);
                    int index = (int)(std::log2(size / 32));
                    m_locations[l][index].push_back(it->second.first);
                    it = m_outfits[l].erase(it);
                    if (--limit <= 0)
                        break;
                } else {
                    ++it;
                }
            }
        }
    }
}


std::string Atlas::getStats() {
    std::stringstream ss;
    for (int l = 0; l < 2; ++l) {
        for (auto& it : m_locations[l]) {
            ss << it.size() << " ";
        }
        ss << "| ";
    }
    ss << "(" << m_size << "|" << g_graphics.getMaxTextureSize() << ")";
    return ss.str();
}
