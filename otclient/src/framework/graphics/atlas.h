#ifndef ATLAS_H
#define ATLAS_H

#include "drawqueue.h"
#include "framebuffer.h"
#include <map>
#include <vector>

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
    bool findSpace(int location, int index);
    void drawOutfit(const Point& location, const DrawQueueOutfit& outfit);

    FrameBufferPtr m_atlas[2];
    std::map<TexturePtr, std::pair<Point, uint32_t>> m_textures[2];
    std::map<size_t, std::pair<Point, uint32_t>> m_outfits[2];
    std::list<Point> m_locations[2][4];
    size_t m_size;
};

extern Atlas g_atlas;

#endif