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

struct DrawQueueOutfit;

struct DrawQueueItem {
    DrawQueueItem() {}
    DrawQueueItem(const Rect& _dest, const TexturePtr& _texture, const Rect& _src, Color _color, float _depth) :
        dest(_dest), texture(_texture), src(_src), color(_color), depth(_depth) {
    }
    virtual ~DrawQueueItem() {};

    virtual DrawQueueOutfit* getOutfit() { return nullptr; }
    virtual void draw(const Rect& location, const Rect& src, bool mark = false);

    Rect dest;
    TexturePtr texture = nullptr;
    Rect src;
    Color color;
    float depth;
    Color mark = Color::black;
    bool cached = true;
};

struct DrawQueueOutfitPattern {
    DrawQueueItem texture;
    std::vector<DrawQueueItem> layers;
};

struct DrawQueueOutfit : public DrawQueueItem {
    DrawQueueOutfit(uint64_t _key, const Rect& _dest, Color _color, float _depth) :
        DrawQueueItem(_dest, nullptr, Rect(), _color, _depth), key(_key) {}
    virtual ~DrawQueueOutfit() {};

    DrawQueueOutfit* getOutfit() override { return this; }
    void draw(const Rect& location, const Rect& src = Rect(), bool mark = false) override;

    uint64_t key;
    DrawQueueItem mount;
    std::vector<DrawQueueOutfitPattern> patterns;
};

class DrawQueue {
public:
    DrawQueue(DrawQueueType type) : m_type(type) {

    }
    ~DrawQueue() {
        reset();
    }
    static void mergeDraw(DrawQueue& q1, DrawQueue& q2);

    void draw();

    DrawQueueItem* add(const Rect& dest, const TexturePtr& texture, const Rect& src, Color color = Color::white, float depth = -1) {
        if (m_blocked)
            return nullptr;
        DrawQueueItem* item = new DrawQueueItem(dest, texture, src, color.opacity(m_opacity), depth == -1 ? m_depth : depth);
        m_queue.push_back(item);
        return item;
    }
    DrawQueueOutfit* addOutfit(uint64_t key, const Rect& dest) {
        if (m_blocked)
            return nullptr;
        DrawQueueOutfit* item = new DrawQueueOutfit(key, dest, Color::white.opacity(m_opacity), m_depth);
        m_queue.push_back(item);
        return item;
    }
    void addBoundingRect(const Rect& dest, const Color& color, int innerLineWidth = 1);


    void setLastItemAsMarked(const Color& color) {
        if (m_queue.empty())
            return;
        m_queue.back()->mark = color;
    }

    DrawQueueOutfit* getLastOutfit() {
        if (m_queue.empty())
            return nullptr;
        return m_queue.back()->getOutfit();
    }

    void reset() {
        for (auto& it : m_queue)
            delete it;
        m_queue.clear();
        m_opacity = 1;
        m_blocked = false;
        m_depth = 0;
    }

    std::vector<DrawQueueItem*>& items() {
        return m_queue;
    }    

    void setAtlas(TexturePtr atlas) {
        m_atlas = atlas;
    }
    TexturePtr getAtlas() {
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

private:
    DrawQueueType m_type;
    std::vector<DrawQueueItem*> m_queue;
    TexturePtr m_atlas = nullptr;
    float m_depth = 0;
    float m_opacity = 1;
    bool m_blocked = false;
};

#endif