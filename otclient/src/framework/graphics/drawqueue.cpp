#include <framework/graphics/drawqueue.h>
#include <framework/graphics/painter.h>
#include <framework/graphics/atlas.h>

void DrawQueueItem::draw(const Rect& location, const Rect& src, bool) {
    if (!texture)
        return;
    g_painter->drawTexturedRect(location, texture, src);
}

void DrawQueueOutfit::draw(const Rect& location, const Rect&, bool mark) {
    auto composition = g_painter->getCompositionMode();

    g_painter->setCompositionMode(Painter::CompositionMode_Normal);
    if (mount.texture)
        g_painter->drawTexturedRect(mount.dest + location.topLeft(), mount.texture, mount.src);
    for (auto& pattern : patterns) {
        g_painter->setCompositionMode(Painter::CompositionMode_Normal);
        if (pattern.texture.texture) 
            g_painter->drawTexturedRect(pattern.texture.dest + location.topLeft(), pattern.texture.texture, pattern.texture.src);
        if (mark)
            continue;
        if (pattern.layer.texture) {
            g_painter->setCompositionMode(Painter::CompositionMode_Multiply);
            if (pattern.layer.texture) {
                Matrix4 mat4;
                for (int x = 0; x < 4; ++x) {
                    mat4(x + 1, 1) = pattern.colors[x].rF();
                    mat4(x + 1, 2) = pattern.colors[x].gF();
                    mat4(x + 1, 3) = pattern.colors[x].bF();
                    mat4(x + 1, 4) = pattern.colors[x].aF();
                }
                g_painter->setDrawOutfitLayersProgram();
                g_painter->setMatrixColor(mat4);
                g_painter->drawTexturedRect(pattern.layer.dest + location.topLeft(), pattern.layer.texture, pattern.layer.src);
                g_painter->resetShaderProgram();
            }
        }
        g_painter->resetColor();
    }

    g_painter->setCompositionMode(composition);
}

void DrawQueue::mergeDraw(DrawQueue& q1, DrawQueue& q2) {
    struct DrawQueueItemDepthSort
    {
        inline bool operator() (const DrawQueueItem* item1, const DrawQueueItem* item2)
        {
            return (item1->depth > item2->depth);
        }
    };

    DrawQueue q(DRAW_QUEUE_MAP);
    for (auto& item : q1.items())
        q.items().push_back(item);
    for (auto& item : q2.items())
        q.items().push_back(item);
    std::stable_sort(q.items().begin(), q.items().end(), DrawQueueItemDepthSort());
    q.draw();
    q.items().clear();
}

void DrawQueue::addBoundingRect(const Rect& dest, const Color& color, int innerLineWidth) {

    int left = dest.left();
    int right = dest.right();
    int top = dest.top();
    int bottom = dest.bottom();
    int width = dest.width();
    int height = dest.height();
    int w = innerLineWidth;

    m_queue.push_back(new DrawQueueItem{ Rect(left, top, width - w, w), nullptr, Rect(0, 0, 32, 32), color.opacity(m_opacity), m_depth }); // top
    m_queue.push_back(new DrawQueueItem{ Rect(right - w + 1, top, w, height - w), nullptr, Rect(0, 0, 32, 32), color.opacity(m_opacity), m_depth }); // top
    m_queue.push_back(new DrawQueueItem{ Rect(left + w, bottom - w + 1, width - w, w), nullptr, Rect(0, 0, 32, 32), color.opacity(m_opacity), m_depth }); // top
    m_queue.push_back(new DrawQueueItem{ Rect(left, top + w, w, height - w), nullptr, Rect(0, 0, 32, 32), color.opacity(m_opacity), m_depth }); // top
}

void DrawQueue::draw() {
    int location = m_type == DRAW_QUEUE_MAP ? 0 : 1;
    bool hasSpace = g_atlas.update(location, *this);
    g_painter->drawQueue(*this);
    //todo fixes for depth less
    for (auto& item : items()) {
        if (item->mark != Color::black) {
            g_painter->setDrawColorOnTextureShaderProgram();
            g_painter->setColor(item->mark);
            g_painter->setDepth(item->depth);
            item->draw(item->dest, item->src, true);
            g_painter->resetShaderProgram();
        }
        if (item->cached)
            continue;
        g_painter->setDepth(item->depth);
        g_painter->setColor(item->color);
        item->draw(item->dest, item->src);
    }
    g_painter->resetColor();
    g_painter->resetDepth();
    //drawUncached();
    //drawMarked();

    if (!hasSpace) {
        g_atlas.reset(location);
    }
}
