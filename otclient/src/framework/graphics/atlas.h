#ifndef ATLAS_H
#define ATLAS_H

#include "drawqueue.h"
#include "framebuffer.h"
#include <map>
#include <vector>

struct CachedAtlasObject {
    CachedAtlasObject(Point& _position, uint32_t _lastUsed, int _index) :
        position(_position), lastUsed(_lastUsed), index(_index) {}
    Point position;
    uint32_t lastUsed;
    int index;
};

class Atlas {
public:
    void init();
    void terminate();
    void reset();
    void update(int location, DrawQueue& queue);
    void clean(bool fastClean);
    std::string getStats();
    TexturePtr getAtlas(int location) { return m_atlas[location]->getTexture(); }

private:
    bool findSpace(int location, int index, bool tryCleaning = true);
    void drawOutfit(const Point& location, const DrawQueueOutfit& outfit);

    FrameBufferPtr m_atlas[2];
    std::map<TexturePtr, CachedAtlasObject> m_textures[2];
    std::map<size_t, CachedAtlasObject> m_outfits[2];
    std::list<Point> m_locations[2][5];
    size_t m_size;
};

extern Atlas g_atlas;

#endif