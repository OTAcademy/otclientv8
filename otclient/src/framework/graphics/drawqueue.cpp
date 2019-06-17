#include <framework/graphics/drawqueue.h>
#include <framework/graphics/painter.h>
#include <framework/graphics/atlas.h>

void DrawQueueOutfit::addBoundingRect(const Rect& dest, const Color& color, int innerLineWidth) {

    int left = dest.left();
    int right = dest.right();
    int top = dest.top();
    int bottom = dest.bottom();
    int width = dest.width();
    int height = dest.height();
    int w = innerLineWidth;

    squares.push_back({ Rect(left, top, width - w, w), nullptr, Rect(0, 0, 32, 32), color, depth }); // top
    squares.push_back({ Rect(right - w + 1, top, w, height - w), nullptr, Rect(0, 0, 32, 32), color, depth }); // top
    squares.push_back({ Rect(left + w, bottom - w + 1, width - w, w), nullptr, Rect(0, 0, 32, 32), color, depth }); // top
    squares.push_back({ Rect(left, top + w, w, height - w), nullptr, Rect(0, 0, 32, 32), color, depth }); // top
}

void DrawQueue::draw() {
    int location = m_type == DRAW_QUEUE_MAP ? 0 : 1;
    g_atlas.update(location, *this);
    g_painter->drawQueue(*this);
    drawUncached();
    drawMarked();
}

void DrawQueue::drawUncached() {
    for (auto& item : m_itemsQueue) {
        if (item.cached)
            continue;
        g_painter->setColor(item.color);
        g_painter->setDepth(item.depth);
        g_painter->drawTexturedRect(item.dest, item.texture, item.src);
    }
    for (auto& item : m_outfitsQueue) {
        if (item.cached)
            continue;
        g_painter->resetColor();
        g_painter->setCompositionMode(Painter::CompositionMode_Normal);
        auto location = item.dest - Point(32, 32);
        if(item.mount.texture)
            g_painter->drawTexturedRect(item.mount.dest + location, item.mount.texture, item.mount.src);
        for (auto& texture : item.textures) {
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
        g_painter->setCompositionMode(Painter::CompositionMode_Normal);
    }
    g_painter->resetColor();

}

void DrawQueue::drawMarked() {
    if (m_markedItemsQueue.empty() && m_markedOutfitsQueue.empty())
        return;

    for (auto& item : m_markedItemsQueue) {
        if (!item.first.texture)
            continue;
        g_painter->setColor(item.second);
        g_painter->setDepth(item.first.depth);
        g_painter->drawColorOnTexturedRect(item.first.dest, item.first.texture, item.first.src);
    }
    for (auto& item : m_markedOutfitsQueue) {
        g_painter->setColor(item.second);
        g_painter->setDepth(item.first.depth);
        auto location = item.first.dest - Point(32, 32);

        if(item.first.mount.texture)
            g_painter->drawColorOnTexturedRect(item.first.mount.dest + location, item.first.mount.texture, item.first.mount.src);
        for (auto& layer : item.first.textures) {
            if(layer.texture.texture) 
                g_painter->drawColorOnTexturedRect(layer.texture.dest + location, layer.texture.texture, layer.texture.src);
        }
    }

    g_painter->resetColor();
}