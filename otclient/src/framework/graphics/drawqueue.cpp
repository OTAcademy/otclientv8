#include <stack>
#include <framework/graphics/drawqueue.h>
#include <framework/graphics/painter.h>
#include <framework/graphics/atlas.h>
#include <framework/graphics/graphics.h>
#include <framework/graphics/framebuffermanager.h>
#include <framework/graphics/textrender.h>
#include <framework/graphics/drawcache.h>
#include <framework/graphics/image.h>

std::shared_ptr<DrawQueue> g_drawQueue;

void DrawQueueItemTextureCoords::draw()
{
    g_painterNew->setColor(m_color);
    g_painterNew->drawTextureCoords(m_coordsBuffer, m_texture);
}

bool DrawQueueItemTextureCoords::cache()
{
    m_texture->update();
    uint64_t hash = 100 + m_texture->getUniqueId();
    bool drawNow = false;
    Point atlasPos = g_atlas.cache(hash, m_texture->getSize(), drawNow);
    if (atlasPos.x < 0) { return false; } // can't be cached
    if (drawNow) { g_drawCache.bind(); draw(atlasPos); }

    int size = m_coordsBuffer.getVertexCount();
    if (!g_drawCache.hasSpace(size))
        return false;

    g_drawCache.addTexturedCoords(m_coordsBuffer, atlasPos, m_color);
    return true;
}

void DrawQueueItemTextureCoords::draw(const Point& pos)
{
    g_painterNew->resetColor();
    g_painterNew->drawTexturedRect(Rect(pos, m_texture->getSize()), m_texture);
}

void DrawQueueItemTexturedRect::draw()
{
    g_painterNew->setColor(m_color);
    g_painterNew->drawTexturedRect(m_dest, m_texture, m_src);
}

bool DrawQueueItemTexturedRect::cache()
{
    if (m_dest.size() > m_src.size()) // upscaling may create artifacts
        return false;

    m_texture->update();
    uint64_t hash = 100 + m_texture->getUniqueId();
    bool drawNow = false;
    Point atlasPos = g_atlas.cache(hash, m_texture->getSize(), drawNow);
    if (atlasPos.x < 0) { return false; } // can't be cached
    if (drawNow) { g_drawCache.bind(); draw(atlasPos); }

    if (!g_drawCache.hasSpace(6))
        return false;

    g_drawCache.addTexturedRect(m_dest, m_src + atlasPos, m_color);
    return true;
}

void DrawQueueItemTexturedRect::draw(const Point& pos)
{
    g_painterNew->resetColor();
    g_painterNew->drawTexturedRect(Rect(pos, m_texture->getSize()), m_texture);
}


bool DrawQueueItemFilledRect::cache()
{
    if (!g_drawCache.hasSpace(6)) return false;
    g_drawCache.addRect(m_dest, m_color);
    return true; 
}

void DrawQueueItemClearRect::draw()
{
    g_painterNew->clearRect(m_color, m_dest);
}

bool DrawQueueItemFillCoords::cache()
{
    int size = m_coordsBuffer.getVertexCount();
    if (!g_drawCache.hasSpace(size))
        return false;

    g_drawCache.addCoords(m_coordsBuffer, m_color);
    return true;
}

void DrawQueueItemText::draw()
{
    g_text.drawText(m_point, m_hash, m_color);
}

void DrawQueueItemTextColored::draw()
{
    g_text.drawColoredText(m_point, m_hash, m_colors);
}

bool DrawQueueItemOutfit::cache()
{
    m_texture->update();
    uint64_t hash = (((uint64_t)m_texture->getUniqueId()) << 48) +
        (((uint64_t)m_src.x()) << 36) +
        (((uint64_t)m_src.y()) << 24) +
        (((uint64_t)m_src.width()) << 12) +
        (((uint64_t)m_src.height())) +
        (((uint64_t)m_colors) * 1125899906842597ULL);
    bool drawNow = false;
    Point atlasPos = g_atlas.cache(hash, m_src.size(), drawNow);
    if (atlasPos.x < 0) { return false; } // can't be cached
    if (drawNow) { g_drawCache.bind(); draw(atlasPos); }

    if (!g_drawCache.hasSpace(6))
        return false;
    
    g_drawCache.addTexturedRect(m_dest, Rect(atlasPos, m_src.size()), m_color);
    return true;
}

void DrawQueueItemOutfit::draw()
{
    if (!m_texture) return;
    Matrix4 mat4;
    for (int x = 0; x < 4; ++x) {
        Color color = Color::getOutfitColor((m_colors >> (x * 8)) & 0xFF);
        mat4(x + 1, 1) = color.rF();
        mat4(x + 1, 2) = color.gF();
        mat4(x + 1, 3) = color.bF();
        mat4(x + 1, 4) = color.aF();
    }
    g_painterNew->setDrawOutfitLayersProgram();
    g_painterNew->setMatrixColor(mat4);
    g_painterNew->setOffset(m_offset);
    g_painterNew->drawTexturedRect(m_dest, m_texture, m_src);
    g_painterNew->resetShaderProgram();
}

void DrawQueueItemOutfit::draw(const Point& pos)
{
    if (!m_texture) return;
    Matrix4 mat4;
    for (int x = 0; x < 4; ++x) {
        Color color = Color::getOutfitColor((m_colors >> (x * 8)) & 0xFF);
        mat4(x + 1, 1) = color.rF();
        mat4(x + 1, 2) = color.gF();
        mat4(x + 1, 3) = color.bF();
        mat4(x + 1, 4) = color.aF();
    }
    g_painterNew->setDrawOutfitLayersProgram();
    g_painterNew->setMatrixColor(mat4);
    g_painterNew->setOffset(m_offset);
    g_painterNew->drawTexturedRect(Rect(pos, m_src.size()), m_texture, m_src);
    g_painterNew->resetShaderProgram();
}

void DrawQueueConditionClip::start(DrawQueue*)
{
    m_prevClip = g_painterNew->getClipRect();
    g_painterNew->setClipRect(m_rect);
}

void DrawQueueConditionClip::end(DrawQueue*)
{
    g_painterNew->setClipRect(m_prevClip);
}

void DrawQueueConditionRotation::start(DrawQueue*)
{
    g_painterNew->pushTransformMatrix();
    g_painterNew->rotate(m_center, m_angle);
}

void DrawQueueConditionRotation::end(DrawQueue*)
{
    g_painterNew->popTransformMatrix();
}

void DrawQueueConditionMark::start(DrawQueue*)
{
    // nothing
}

void DrawQueueConditionMark::end(DrawQueue* queue)
{
    g_painterNew->setDrawColorOnTextureShaderProgram();
    g_painterNew->setColor(m_color);
    for (size_t i = m_start; i < m_end; ++i) {
        DrawQueueItemTexturedRect* texture = dynamic_cast<DrawQueueItemTexturedRect*>(queue->m_queue[i]);
        if (texture)
            g_painterNew->drawTexturedRect(texture->m_dest, texture->m_texture, texture->m_src);
    }
    g_painterNew->resetShaderProgram();
}

void DrawQueue::setFrameBuffer(const Rect& dest, const Size& size, const Rect& src)
{
    m_useFrameBuffer = true;
    m_frameBufferSize = size;
    m_frameBufferDest = dest;
    m_frameBufferSrc = src;
    size_t max_size = std::max(m_frameBufferSize.width(), m_frameBufferSize.height());
    while(max_size > 2048u) {
        max_size /= 2;
        m_scaling /= 2.f;
    }
    if (m_scaling < 0.99f) {
        m_frameBufferSize = Size(2048, 2048);
        m_frameBufferSrc = m_frameBufferSrc * m_scaling;
    }
}

void DrawQueue::addText(BitmapFontPtr font, const std::string& text, const Rect& screenCoords, Fw::AlignmentFlag align, const Color& color)
{
    if (!font || text.empty()) return;
    uint64_t hash = g_text.addText(font, text, screenCoords.size(), align);
    m_queue.push_back(new DrawQueueItemText(screenCoords.topLeft(), font->getTexture(), hash, color));
}

void DrawQueue::addColoredText(BitmapFontPtr font, const std::string& text, const Rect& screenCoords, Fw::AlignmentFlag align, const std::vector<std::pair<int, Color>>& colors)
{
    if (!font || text.empty()) return;
    uint64_t hash = g_text.addText(font, text, screenCoords.size(), align);
    m_queue.push_back(new DrawQueueItemTextColored(screenCoords.topLeft(), font->getTexture(), hash, colors));
}

void DrawQueue::correctOutfit(const Rect& dest, int fromPos)
{
    std::vector<Rect*> rects;
    for (size_t i = fromPos; i < m_queue.size(); ++i) {
        if (DrawQueueItemTexturedRect* texture = dynamic_cast<DrawQueueItemTexturedRect*>(m_queue[i]))
            rects.push_back(&texture->m_dest);
    }

    int x1 = 0, y1 = 1, x2 = 0, y2 = 0;
    for (auto& rect : rects) {
        x1 = std::min<int>(x1, rect->left());
        y1 = std::min<int>(y1, rect->top());
        x2 = std::max<int>(x2, rect->right());
        y2 = std::max<int>(y2, rect->bottom());
    }
    if (x1 == x2 || y1 == y2) return;
    float scale = std::min<float>((float)dest.height() / (y2 - y1), (float)dest.width() / (x2 - x1));
    for (auto& rect : rects) {
        int x = rect->left() - x1, y = rect->top() - y1; // offset
        *rect = Rect(dest.left() + x * scale, dest.top() + y * scale, rect->size() * scale);
    }
}

void DrawQueue::draw(DrawType drawType)
{
    size_t start = 0;
    size_t end = m_queue.size();
    if (drawType == DRAW_BEFORE_MAP) {
        end = mapPosition;
    } else if (drawType == DRAW_AFTER_MAP) {
        start = mapPosition;
    }

    std::sort(m_conditions.begin(), m_conditions.end(), [](const DrawQueueCondition* a, const DrawQueueCondition* b) -> bool {
        return a->m_start == b->m_start ? a->m_end < b->m_end : a->m_start < b->m_start;
    });

    Size originalResolution = g_painterNew->getResolution();
    if (m_scaling > 0.f && m_scaling < 0.99f) {
        Size resolution = originalResolution * (1.f / m_scaling);
        Matrix3 projectionMatrix = { 
            2.0f / resolution.width(),  0.0f,                      0.0f,
            0.0f,                    -2.0f / resolution.height(),  0.0f,
            -1.0f,                     1.0f,                      1.0f 
        };
        g_painterNew->setProjectionMatrix(projectionMatrix);
    }

    auto condition = m_conditions.begin();
    std::stack<DrawQueueCondition*> activeConditions;
    // skip conditions
    while (condition != m_conditions.end() && (*condition)->m_end <= start)
        ++condition;
    // execute conditions & draw
    for (size_t i = start; i < end; ++i) {
        while (condition != m_conditions.end() && (*condition)->m_start <= i) {
            g_drawCache.draw();
            (*condition)->start(this);
            activeConditions.push(*condition);
            ++condition;
        }
        while (!activeConditions.empty() && activeConditions.top()->m_end <= i) {
            g_drawCache.draw();
            activeConditions.top()->end(this);
            activeConditions.pop();
        }

        if (!m_queue[i]->cache()) {
            g_drawCache.draw();
            if (!m_queue[i]->cache()) { // try to cache again, now g_drawCache should be empty, maybe there's new space
                m_queue[i]->draw();
            }
        }
        if (g_drawCache.getSize() >= g_drawCache.HALF_MAX_SIZE) {
            g_drawCache.draw();
        }
    }
    g_drawCache.draw();
    // end all actibe conditions
    while (!activeConditions.empty()) {
        activeConditions.top()->end(this);
        activeConditions.pop();
    }

    g_painterNew->setResolution(originalResolution);
    g_painterNew->resetState();
    g_graphics.checkForError(__FUNCTION__, __FILE__, __LINE__);
}
