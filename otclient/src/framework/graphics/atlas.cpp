#include "atlas.h"
#include "framebuffermanager.h"
#include "painter.h"
#include "graphics.h"
#include <framework/core/clock.h>
#include <framework/platform/platform.h>

Atlas g_atlas;

void Atlas::init() {
    m_size = std::min<size_t>(4096, g_graphics.getMaxTextureSize());
    double memory = g_platform.getTotalSystemMemory() / (1024. * 1024.);
    if (memory > 128 && memory < 4096 && m_size > 2048) { // old computer, use smaller texture
        g_logger.info("[Atlas] Settings for old computer");
        m_size = 2048;
    }
    g_logger.info(stdext::format("[Atlas] Texture size is: %ix%i (max: %ix%i)", m_size, m_size, g_graphics.getMaxTextureSize(), g_graphics.getMaxTextureSize()));

    for (int l = 0; l < 2; ++l) {
        m_atlas[l] = g_framebuffers.createFrameBuffer(false);
        m_atlas[l]->resize(Size(m_size, m_size));
    }
    reset(0);
    reset(1);
}

void Atlas::reset(int location) {
    if (!m_atlas[location] || location < 0 || location >= 2)
        return;
    for (int j = 0; j < 5; ++j)
        m_locations[location][j].clear();
    m_outfits[location].clear();
    m_textures[location].clear();

    for (size_t x = 0; x < m_size; x += 512) {
        for (size_t y = 0; y < m_size; y += 512) {
            m_locations[location][4].push_back(Point(x, y));
        }
    }
    // create 32x32 white square
    findSpace(location, 0);
    assert(!m_locations[location][0].empty());
    m_locations[location][0].pop_front();
    m_atlas[location]->bind();
    g_painter->setAlphaWriting(true);
    g_painter->setCompositionMode(Painter::CompositionMode_Replace);
    g_painter->setColor(Color::alpha);
    g_painter->drawFilledRect(Rect(0, 0, m_atlas[location]->getSize()));
    g_painter->setColor(Color::white);
    g_painter->drawFilledRect(Rect(0, 0, Size(32, 32)));
    m_atlas[location]->release();
}


void Atlas::terminate() {
    for(auto& it : m_atlas)
        it = nullptr;
    for(auto& it : m_textures)
        it.clear();
}

bool Atlas::update(int location, DrawQueue& queue) {
    if (location >= 2 || !m_atlas[location])
        return true;

    static const size_t sizes[5] = { 32, 64, 128, 256, 512 };
    auto& textures = m_textures[location];
    auto& outfits = m_outfits[location];
    auto atlasTexture = m_atlas[location]->getTexture();
    bool hasSpace = true;

    bool bound = false;
    for (auto& item : queue.items()) {
        auto& tex = item->texture;
        if (tex == atlasTexture) 
            continue; // already processed
        DrawQueueOutfit* outfit = item->getOutfit();
        if (!tex && !outfit) { 
            item->texture = atlasTexture;
            item->src = Rect(0, 0, 4, 4);
            continue; // solid color
        }

        size_t size = outfit ? std::max<size_t>(outfit->dest.width(), outfit->dest.height())
            : std::max<size_t>(tex->getSize().width(), tex->getSize().height());
        if (size < 32)
            size = 32;
        if (size > 512) {
            item->cached = false;
            hasSpace = false;
            continue; // too big to be cached
        }

        int index = 0; // this is optimization, std::log2 was taking 1-2% cpu
        if (size > 256)
            index = 4;
        else if (size > 128)
            index = 3;
        else if (size > 64)
            index = 2;
        else if (size > 32)
            index = 1;

        if (outfit) {
            auto it = outfits.find(outfit->key);
            if(it == outfits.end()) {
                if (m_locations[location][index].empty() && !findSpace(location, index)) {
                    item->cached = false;
                    hasSpace = false;
                    continue;
                }

                Point drawLocation = m_locations[location][index].front();
                m_locations[location][index].pop_front();
                it = outfits.emplace(outfit->key, CachedAtlasObject(drawLocation, g_clock.millis(), index)).first;

                if (!bound) {
                    bound = true;
                    m_atlas[location]->bind();
                    g_painter->setAlphaWriting(true);
                    g_painter->setDepthFunc(Painter::DepthFunc_ALWAYS);
                    g_painter->setCompositionMode(Painter::CompositionMode_Replace);
                }
                g_painter->clearRect(Color::alpha, Rect(drawLocation, Size(size, size)));
                outfit->draw(Rect(drawLocation, outfit->dest.size()));
            }

            it->second.lastUsed = g_clock.millis();
            outfit->texture = atlasTexture;
            outfit->src = Rect(it->second.position, outfit->dest.size());
            continue;
        }

        auto it = textures.find(tex);
        if (it == textures.end()) {
            if (m_locations[location][index].empty() && !findSpace(location, index)) {
                item->cached = false; // no more space in atlas
                continue;
            }

            Point drawLocation = m_locations[location][index].front();
            m_locations[location][index].pop_front();
            it = textures.emplace(item->texture, CachedAtlasObject(drawLocation, g_clock.millis(), index)).first;

            if (!bound) {
                bound = true;
                m_atlas[location]->bind();
                g_painter->setAlphaWriting(true);
                g_painter->setDepthFunc(Painter::DepthFunc_ALWAYS);
                g_painter->setCompositionMode(Painter::CompositionMode_Replace);
            }

            g_painter->clearRect(Color::alpha, Rect(drawLocation, Size(size, size)));
            item->draw(Rect(drawLocation, tex->getSize()), Rect(0, 0, tex->getSize()));
        }

        it->second.lastUsed = g_clock.millis();
        item->texture = atlasTexture;
        item->src = Rect(item->src.topLeft() + it->second.position, item->src.size());
    }
    
    if (bound) {
        m_atlas[location]->release();
    }
    queue.setAtlas(m_atlas[location]->getTexture());
    return hasSpace;
}

bool Atlas::findSpace(int location, int index) {
    static const size_t sizes[5] = { 32, 64, 128, 256, 512 };
    if (location >= 2 || index >= 4) {
        return false;
    }
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

void Atlas::clean(bool fastClean) {
    return; // replaced by reset
    if (!m_atlas[0])
        return;

    for (int l = 0; l < 2; ++l) {
        size_t count = 0;
        for (int i = 0; i < 5; ++i) {
            count += m_locations[l][i].size() * (i + 1) * (i + 1);
        }
        if ((count < 100 && fastClean) || count < 1000) {
            int limit = fastClean ? 10 : 100;
            for (auto it = m_textures[l].begin(); it != m_textures[l].end(); ) {
                if (it->second.lastUsed + (fastClean ? 2000 : 60000) < g_clock.millis()) {
                    m_locations[l][it->second.index].push_back(it->second.position);
                    it = m_textures[l].erase(it);
                    if (--limit <= 0)
                        break;
                } else {
                    ++it;
                }
            }
            limit = fastClean ? 10 : 100;
            for (auto it = m_outfits[l].begin(); it != m_outfits[l].end(); ) {
                if (it->second.lastUsed + (fastClean ? 2000 : 60000) < g_clock.millis()) {
                    m_locations[l][it->second.index].push_back(it->second.position);
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
