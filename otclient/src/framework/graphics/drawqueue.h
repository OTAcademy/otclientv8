#ifndef DRAWQUEUE_H
#define DRAWQUEUE_H

#include <vector>
#include <framework/graphics/declarations.h>
#include <framework/graphics/coordsbuffer.h>
#include <framework/graphics/paintershaderprogram.h>
#include <framework/graphics/texture.h>
#include <framework/graphics/colorarray.h>
#include <framework/graphics/deptharray.h>

enum DrawQueueType {
    DRAW_QUEUE_MAP = 0,
    DRAW_QUEUE_CREATURES = 1,
    DRAW_QUEUE_CREATURES_INFO = 2
};

struct DrawQueueItem {
    Rect dest;
    TexturePtr texture = nullptr;
    Rect src;
    Color color;
    float depth;
    bool cached = true;
};

struct DrawQueueOutfitTextures {
    DrawQueueItem texture;
    std::vector<DrawQueueItem> layers;
};

struct DrawQueueOutfit {
    DrawQueueOutfit(size_t _key, Point _dest, float _opacity, float _depth) : key(_key), dest(_dest), opacity(_opacity), depth(_depth) {}
    void addBoundingRect(const Rect& dest, const Color& color, int innerLineWidth = 1);

    size_t key;
    Point dest;
    float opacity;
    float depth;
    bool cached = true;

    std::vector<DrawQueueItem> squares;
    DrawQueueItem mount;
    std::vector<DrawQueueOutfitTextures> textures;
};

class DrawQueue {
public:
    DrawQueue(DrawQueueType type) : m_type(type) {}

    void draw();

    void add(const Rect& dest, const TexturePtr& texture, const Rect& src, Color color = Color::white, float depth = -1) {
        if (m_blocked)
            return;
        m_itemsQueue.push_back({ dest, texture, src, color.opacity(m_opacity), depth == -1 ? m_depth : depth });
    }
    void add(const DrawQueueItem& item) {
        m_itemsQueue.push_back(item);
    }

    void addOutfit(size_t key, Point dest) {
        m_outfitsQueue.push_back(DrawQueueOutfit(key, dest, m_opacity, m_depth));
    }

    void addMarked(bool outfit, const Color& color) {
        if (outfit && !m_outfitsQueue.empty())
            m_markedOutfitsQueue.push_back(std::make_pair(m_outfitsQueue.back(), color));
        else if(!m_itemsQueue.empty())
            m_markedItemsQueue.push_back(std::make_pair(m_itemsQueue.back(), color));
    }

    DrawQueueOutfit& getLastOutfit() {
        assert(!m_outfitsQueue.empty());
        return m_outfitsQueue.back();
    }

    void reset() {
        m_itemsQueue.clear();
        m_outfitsQueue.clear();
        m_markedItemsQueue.clear();
        m_markedOutfitsQueue.clear();
        m_opacity = 1;
        m_blocked = false;
        m_depth = 0;
    }

    std::vector<DrawQueueItem>& items() {
        return m_itemsQueue;
    }
    std::vector<DrawQueueOutfit>& outfits() {
        return m_outfitsQueue;
    }
    std::vector<std::pair<DrawQueueItem, Color>>& markedItems() {
        return m_markedItemsQueue;
    }
    std::vector<std::pair<DrawQueueOutfit, Color>>& markedOutfits() {
        return m_markedOutfitsQueue;
    }

    void setAtlas(TexturePtr atlas) {
        m_atlas = atlas;
    }
    TexturePtr atlas() {
        return m_atlas;
    }

    void setDepth(float depth) {
        m_depth = depth;
    }
    float getDepth() {
        return m_depth;
    }
    void setOpacity(float opacity) {
        m_opacity = opacity;
    }
    float getOpacity() {
        return m_opacity;
    }

    void block() {
        m_blocked = true;
    }
    void unblock() {
        m_blocked = false;
    }
    void setBlocked(bool value) {
        m_blocked = value;
    }
    bool isBlocked() {
        return m_blocked;
    }

    void drawUncached();
    void drawMarked();

private:
    std::vector<DrawQueueItem> m_itemsQueue;
    std::vector<DrawQueueOutfit> m_outfitsQueue;
    std::vector<std::pair<DrawQueueItem, Color>> m_markedItemsQueue;
    std::vector<std::pair<DrawQueueOutfit, Color>> m_markedOutfitsQueue;
    DrawQueueType m_type;
    TexturePtr m_atlas = nullptr;
    float m_depth = 0;
    float m_opacity = 1;
    bool m_blocked = false;
};

#endif