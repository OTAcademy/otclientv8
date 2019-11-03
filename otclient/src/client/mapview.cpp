/*
 * Copyright (c) 2010-2017 OTClient <https://github.com/edubart/otclient>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "mapview.h"

#include "creature.h"
#include "map.h"
#include "tile.h"
#include "statictext.h"
#include "animatedtext.h"
#include "missile.h"
#include "shadermanager.h"
#include "lightview.h"
#include "localplayer.h"
#include "game.h"
#include "spritemanager.h"

#include <framework/graphics/graphics.h>
#include <framework/graphics/image.h>
#include <framework/graphics/framebuffermanager.h>
#include <framework/core/eventdispatcher.h>
#include <framework/core/application.h>
#include <framework/core/resourcemanager.h>
#include <framework/graphics/texturemanager.h>
#include <framework/graphics/atlas.h>

#include <framework/util/extras.h>
#include <framework/core/adaptiverenderer.h>

MapView::MapView()
{
    m_lockedFirstVisibleFloor = -1;
    m_cachedFirstVisibleFloor = 7;
    m_cachedLastVisibleFloor = 7;
    m_fadeOutTime = 0;
    m_fadeInTime = 0;
    m_minimumAmbientLight = 0;
    m_optimizedSize = Size(g_map.getAwareRange().horizontal(), g_map.getAwareRange().vertical()) * g_sprites.spriteSize();

    m_mapbuffer = g_framebuffers.createFrameBuffer(true);
    m_framebuffer = g_framebuffers.createFrameBuffer(false);
    m_framebuffer->setSmooth(true);

    setVisibleDimension(Size(15, 11));

    m_shader = g_shaders.getDefaultMapShader();
}

MapView::~MapView()
{
#ifndef NDEBUG
    assert(!g_app.isTerminated());
#endif
}

void MapView::drawTiles(bool map, bool creatures, bool isFading, const TilePtr& crosshairTile, bool draw) {
    Position cameraPosition = getCameraPosition();
    auto itm = m_cachedVisibleTiles.begin();
    auto end = m_cachedVisibleTiles.end();

    if (m_lightView && map)
        m_lightView->resetMapLight();
    if (m_lightView && creatures)
        m_lightView->resetCreaturesLight();

    if(map)
        drawQueueMap.reset();
    if(creatures)
        drawQueueCreatures.reset();

    for (int z = m_cachedLastVisibleFloor; z >= (isFading ? m_cachedFirstFadingFloor : m_cachedFirstVisibleFloor); --z) {
        float fading = 1.0;
        if (isFading && m_floorFading > 0) {
            fading = stdext::clamp<float>((float)m_fadingFloorTimers[z].elapsed_millis() / (float)m_floorFading, 0.f, 1.f);
            if (z < m_cachedFirstVisibleFloor)
                fading = 1.0 - fading;
        }
        if (fading == 0) // reached last visible floor
            break;

        if (isFading) {
            drawQueueMap.reset();
            drawQueueCreatures.reset();
        }

        drawQueueMap.setOpacity(fading);
        drawQueueCreatures.setOpacity(fading);

        if (m_lightView)
            m_lightView->setFloor(z);

        for (; itm != end; ++itm) {
            Position tilePos = itm->first->getPosition();
            if (tilePos.z != z)
                break;

            if (itm->second && !isFading) // render only light
                drawQueueMap.block();

            if (map && m_lightView && itm->first->getGround() && !itm->first->getGround()->isTranslucent()) {
                m_lightView->addGround(transformPositionTo2D(tilePos, cameraPosition));
            }

            if (map)
                itm->first->drawItems(transformPositionTo2D(tilePos, cameraPosition), drawQueueMap, m_lightView.get());
            if(creatures)
                itm->first->drawCreatures(transformPositionTo2D(tilePos, cameraPosition), drawQueueCreatures, m_lightView.get());

            if (itm->second && !isFading)
                drawQueueMap.unblock();
        }
        if (creatures) {
            drawQueueCreatures.setDepth(m_floorDepth[z]);
            for (const MissilePtr& missile : g_map.getFloorMissiles(z)) {
                missile->newDraw(transformPositionTo2D(missile->getPosition(), cameraPosition), drawQueueCreatures, m_lightView.get());
            }

            if (crosshairTile && m_crosshair) {
                drawQueueCreatures.setDepth(m_floorDepth[crosshairTile->getPosition().z]);
                drawQueueCreatures.add(Rect(transformPositionTo2D(crosshairTile->getPosition(), cameraPosition), 
                                            transformPositionTo2D(crosshairTile->getPosition(), cameraPosition) + Otc::TILE_PIXELS - 1),
                                       m_crosshair, Rect(0, 0, m_crosshair->getSize()));
            }
        }

        if (isFading) {
            drawQueueMap.draw();
            drawQueueCreatures.draw();
        }
    }

    if (isFading)
        return;
    
    if (!draw)
        return;

    if (map)
        drawQueueMap.draw();
    if (creatures)
        drawQueueCreatures.draw();
}

void MapView::drawTileTexts(const Rect& rect, const Rect& srcRect)
{
    Position cameraPosition = getCameraPosition();
    Point drawOffset = srcRect.topLeft();
    float horizontalStretchFactor = rect.width() / (float)srcRect.width();
    float verticalStretchFactor = rect.height() / (float)srcRect.height();

    auto player = g_game.getLocalPlayer();
    for (auto& tile : m_cachedVisibleTiles) {
        Position tilePos = tile.first->getPosition();
        if (tilePos.z != player->getPosition().z) continue;        

        Point p = transformPositionTo2D(tilePos, cameraPosition) - drawOffset;
        p.x *= horizontalStretchFactor;
        p.y *= verticalStretchFactor;
        p += rect.topLeft();
        p.y += 5;

        tile.first->drawTexts(p);
    }
}


void MapView::draw(const Rect& rect, const TilePtr& crosshairTile) {
    Position cameraPosition = getCameraPosition();
    if (m_mustUpdateVisibleTilesCache) {
        updateVisibleTilesCache();
    }

    if (g_game.getFeature(Otc::GameForceLight)) {
        if(!m_lightView) {
            setDrawLights(true);
        }
        m_minimumAmbientLight = 0.05f;
    }

    Rect srcRect = calcFramebufferSource(rect.size());
    if (m_lightView) {
        Light ambientLight;
        if (cameraPosition.z <= Otc::SEA_FLOOR) {
            ambientLight = g_map.getLight();
        } else {
            ambientLight.color = 215;
            ambientLight.intensity = 0;
        }
        ambientLight.intensity = std::max<int>(m_minimumAmbientLight * 255, ambientLight.intensity);
        m_lightView->setGlobalLight(ambientLight);
    }

    bool isFading = false;
    if (m_floorFading > 0 && g_adaptiveRenderer.allowFading() && !m_lightView) {
        for (int z = m_cachedLastVisibleFloor; z >= m_cachedFirstFadingFloor; --z) {
            if (m_fadingFloorTimers[z].elapsed_millis() <= m_floorFading) {
                isFading = true;
            }
        }
    }

    bool updateMap = m_mustDrawVisibleTilesCache || m_mapRenderTimer.elapsed_millis() >= g_adaptiveRenderer.mapRenderInterval();
    m_mustDrawVisibleTilesCache = false;

    g_painter->resetDraws();

    if (g_graphics.canUseDepth() && !g_game.getFeature(Otc::GameSpritesAlphaChannel)) {
        if (updateMap && !isFading) {
            AutoStat s(STATS_RENDER, "UpdateMap");
            m_mapRenderTimer.restart();
            m_mapbuffer->bind();
            g_painter->setDepthFunc(Painter::DepthFunc_LEQUAL);
            g_painter->setAlphaWriting(true);
            g_painter->clear(Color::alpha);
            drawTiles(true, false, false, crosshairTile);
            m_mapbuffer->release();
        }
        {
            AutoStat s(STATS_RENDER, "UpdateCreatures");
            m_framebuffer->bind(m_mapbuffer);
            g_painter->setAlphaWriting(true);
            if (isFading) {
                g_painter->setDepthFunc(Painter::DepthFunc_LEQUAL);
                g_painter->clear(Color::alpha);
            } else {
                g_painter->setCompositionMode(Painter::CompositionMode_Replace);
                m_mapbuffer->draw();
                g_painter->setDepthFunc(Painter::DepthFunc_LEQUAL_READ);
                g_painter->resetCompositionMode();
            }
            drawTiles(isFading, true, isFading, crosshairTile);
            m_framebuffer->release();
        }
    } else {
        {
            AutoStat s(STATS_RENDER, "UpdateMapNoDepth");
            if (updateMap)
                drawTiles(true, false, false, crosshairTile, false);
            drawTiles(false, true, false, crosshairTile, false);
        }
        AutoStat s(STATS_RENDER, "DrawMapNoDepth");
        m_framebuffer->bind();
        g_painter->setAlphaWriting(true);
        g_painter->clear(Color::alpha);
        DrawQueue::mergeDraw(drawQueueMap, drawQueueCreatures);
        m_framebuffer->release();
    }

    float fadeOpacity = 1.0f;
    if(!m_shaderSwitchDone && m_fadeOutTime > 0) {
        fadeOpacity = 1.0f - (m_fadeTimer.timeElapsed() / m_fadeOutTime);
        if(fadeOpacity < 0.0f) {
            m_shader = m_nextShader;
            m_nextShader = nullptr;
            m_shaderSwitchDone = true;
            m_fadeTimer.restart();
        }
    }

    if(m_shaderSwitchDone && m_shader && m_fadeInTime > 0)
        fadeOpacity = std::min<float>(m_fadeTimer.timeElapsed() / m_fadeInTime, 1.0f);

    Point drawOffset = srcRect.topLeft();
    if(m_shader && g_painter->hasShaders() && g_graphics.shouldUseShaders()) 
    {
        Rect framebufferRect = Rect(0,0, m_drawDimension * g_sprites.spriteSize());
        Point center = srcRect.center();
        Point globalCoord = Point(cameraPosition.x - m_drawDimension.width()/2, -(cameraPosition.y - m_drawDimension.height()/2)) * g_sprites.spriteSize();
        m_shader->bind();
        m_shader->setUniformValue(ShaderManager::MAP_CENTER_COORD, center.x / (float)framebufferRect.width(), 1.0f - center.y / (float)framebufferRect.height());
        m_shader->setUniformValue(ShaderManager::MAP_GLOBAL_COORD, globalCoord.x / (float)framebufferRect.height(), globalCoord.y / (float)framebufferRect.height());
        m_shader->setUniformValue(ShaderManager::MAP_ZOOM, 1.0f);
        g_painter->setShaderProgram(m_shader);
    } 

    g_painter->setAlphaWriting(false);
    g_painter->resetDepthFunc();
    g_painter->resetCompositionMode();
    g_painter->resetColor();
    g_painter->setOpacity(fadeOpacity);
    g_painter->setDepthFunc(Painter::DepthFunc_ALWAYS);
    g_painter->setDepth(2000);
    {
        AutoStat s(STATS_RENDER, "Framebuffer draw");
        m_framebuffer->draw(rect, srcRect);
    }
    g_painter->resetShaderProgram();
    g_painter->setAlphaWriting(true);
    g_painter->resetOpacity();

    // this could happen if the player position is not known yet
    if(!cameraPosition.isValid())
        return;

    float horizontalStretchFactor = rect.width() / (float)srcRect.width();
    float verticalStretchFactor = rect.height() / (float)srcRect.height();

    drawQueueCreaturesInfo.reset();
    drawQueueCreaturesInfo.setDepth(1500);
    g_painter->setDepth(1500);
    for (const CreaturePtr& creature : g_map.getSpectatorsInRangeEx(cameraPosition, false, m_visibleDimension.width() / 2, m_visibleDimension.width() / 2 + 1, m_visibleDimension.height() / 2, m_visibleDimension.height() / 2 + 1)) {
        if (!creature->canBeSeen())
            continue;

        PointF jumpOffset = creature->getJumpOffset();
        Point creatureOffset = Point(16 - creature->getDisplacementX(), -creature->getDisplacementY() - 2);
        Position pos = creature->getPrewalkingPosition();
        Point p = transformPositionTo2D(pos, cameraPosition) - drawOffset;
        p += (creature->getDrawOffset() + creatureOffset) - Point(stdext::round(jumpOffset.x), stdext::round(jumpOffset.y));
        p.x = p.x * horizontalStretchFactor;
        p.y = p.y * verticalStretchFactor;
        p += rect.topLeft();

        int flags = 0;
        if (m_drawNames) { flags = Otc::DrawNames; }
        if (m_drawHealthBarsOnTop) { flags |= Otc::DrawBarsOnTop; }
        if (!creature->isLocalPlayer() || m_drawPlayerBars) {
            if (m_drawHealthBars) { flags |= Otc::DrawBars; }
            if (m_drawManaBar) { flags |= Otc::DrawManaBar; }
        }
        // drawInformation draws creature names, not cached
        creature->drawInformation(p, g_map.isCovered(pos, m_cachedFirstVisibleFloor), rect, flags, drawQueueCreaturesInfo);
    }

    drawQueueCreaturesInfo.draw();

    g_painter->setDepthFunc(Painter::DepthFunc_ALWAYS_READ);
    if (m_lightView) {
        AutoStat s(STATS_RENDER, "Lights");
        m_lightView->draw(rect, srcRect);
    }

    g_painter->setDepthFunc(Painter::DepthFunc_LESS);
    g_painter->setDepth(1000);
    if(m_drawTexts) {
        AutoStat s(STATS_RENDER, "Texts");
        drawTexts(rect, srcRect);
    }

    g_painter->setDepthFunc(Painter::DepthFunc_None);

    drawTileTexts(rect, srcRect);

    if (g_extras.debugRender) {
        static CachedText text;
        g_painter->resetColor();
        text.setText(stdext::format("Reneding %i things (%i gpu calls)\n%i x %i -> %i x %i -> %i x %i", g_painter->draws(), g_painter->calls(), m_framebuffer->getSize().width(), m_framebuffer->getSize().height(), 
            srcRect.width(), srcRect.height(), rect.width(), rect.height()));
        text.draw(rect + Point(0, -200));
    }
}

void MapView::drawTexts(const Rect& rect, const Rect& srcRect)
{
    Position cameraPosition = getCameraPosition();
    Point drawOffset = srcRect.topLeft();
    float horizontalStretchFactor = rect.width() / (float)srcRect.width();
    float verticalStretchFactor = rect.height() / (float)srcRect.height();
    int limit = g_adaptiveRenderer.textsLimit();

    for (int i = 0; i < 2; ++i) {
        for (const StaticTextPtr& staticText : g_map.getStaticTexts()) {
            Position pos = staticText->getPosition();

            if (pos.z != cameraPosition.z && staticText->getMessageMode() == Otc::MessageNone)
                continue;
            if ((staticText->getMessageMode() != Otc::MessageSay && staticText->getMessageMode() != Otc::MessageYell)) {
                if (i == 0)
                    continue;
            }
            else if (i == 1)
                continue;

            Point p = transformPositionTo2D(pos, cameraPosition) - drawOffset + Point(8, 0);
            p.x *= horizontalStretchFactor;
            p.y *= verticalStretchFactor;
            p += rect.topLeft();
            staticText->drawText(p, rect);
            if (--limit == 0)
                break;
        }
    }

    limit = g_adaptiveRenderer.textsLimit();
    for (const AnimatedTextPtr& animatedText : g_map.getAnimatedTexts()) {
        Position pos = animatedText->getPosition();

        if (pos.z != cameraPosition.z)
            continue;

        Point p = transformPositionTo2D(pos, cameraPosition) - drawOffset + Point(8, 0);
        p.x *= horizontalStretchFactor;
        p.y *= verticalStretchFactor;
        p += rect.topLeft();
        animatedText->drawText(p, rect);
        if (--limit == 0)
            break;
    }
}


void MapView::updateVisibleTilesCache()
{
    AutoStat s(STATS_RENDER, "updateVisibleTilesCache");

    int prevFirstVisibleFloor = m_cachedFirstVisibleFloor;
    m_cachedFirstVisibleFloor = calcFirstVisibleFloor(false);
    m_cachedFirstFadingFloor = calcFirstVisibleFloor(true);
    m_cachedLastVisibleFloor = calcLastVisibleFloor();

    assert(m_cachedFirstVisibleFloor >= 0 && m_cachedLastVisibleFloor >= 0 &&
            m_cachedFirstVisibleFloor <= Otc::MAX_Z && m_cachedLastVisibleFloor <= Otc::MAX_Z);

    if(m_cachedLastVisibleFloor < m_cachedFirstVisibleFloor)
        m_cachedLastVisibleFloor = m_cachedFirstVisibleFloor;

    m_mustUpdateVisibleTilesCache = false;

    // there is no tile to render on invalid positions
    Position cameraPosition = getCameraPosition();
    if (!cameraPosition.isValid()) {
        m_mustDrawVisibleTilesCache = true;
        return;
    }

    // fading
    if (!m_lastCameraPosition.isValid() || m_lastCameraPosition.z != cameraPosition.z || m_lastCameraPosition.distance(cameraPosition) >= 3) { 
        for (int iz = m_cachedLastVisibleFloor; iz >= m_cachedFirstFadingFloor; --iz) {
            m_fadingFloorTimers[iz].restart(m_floorFading * 1000);
        }
    } else if (prevFirstVisibleFloor < m_cachedFirstVisibleFloor) { // showing new floor
        for (int iz = prevFirstVisibleFloor; iz < m_cachedFirstVisibleFloor; ++iz) {
            int shift = std::max<int>(0, m_floorFading - m_fadingFloorTimers[iz].elapsed_millis());
            m_fadingFloorTimers[iz].restart(shift * 1000);
        }
    } else if (prevFirstVisibleFloor > m_cachedFirstVisibleFloor) { // hiding floor
        for (int iz = m_cachedFirstVisibleFloor; iz < prevFirstVisibleFloor; ++iz) {
            int shift = std::max<int>(0, m_floorFading - m_fadingFloorTimers[iz].elapsed_millis());
            m_fadingFloorTimers[iz].restart(shift * 1000);
        }
    }

    m_lastCameraPosition = cameraPosition;

    const int numDiagonals = m_drawDimension.width() + m_drawDimension.height() - 1;
    float depth = Otc::MAX_DEPTH;
    m_mustDrawVisibleTilesCache = false;

    size_t i = 0;
    for (int iz = m_cachedLastVisibleFloor; iz >= (m_floorFading ? m_cachedFirstFadingFloor : m_cachedFirstVisibleFloor); --iz) {
        for (int diagonal = 0; diagonal < numDiagonals && !m_mustDrawVisibleTilesCache; ++diagonal) {
            int advance = std::max<int>(diagonal - m_drawDimension.height(), 0);
            for (int iy = diagonal - advance, ix = advance; iy >= 0 && ix < m_drawDimension.width() && !m_mustDrawVisibleTilesCache; --iy, ++ix) {
                Position tilePos = cameraPosition.translated(ix - m_virtualCenterOffset.x, iy - m_virtualCenterOffset.y);
                tilePos.coveredUp(cameraPosition.z - iz);
                if (const TilePtr& tile = g_map.getTile(tilePos)) {
                    if (!tile->isDrawable())
                        continue;
                    if (i >= m_cachedVisibleTiles.size()) {
                        m_mustDrawVisibleTilesCache = true;
                        break;
                    }
                    if (m_cachedVisibleTiles[i].first != tile || m_cachedVisibleTiles[i].second != g_map.isCompletelyCovered(tilePos, m_cachedFirstVisibleFloor)) {
                        m_mustDrawVisibleTilesCache = true;
                        break;
                    }
                    i += 1;
                }

            }
        }
    }
    if (i != m_cachedVisibleTiles.size())
        m_mustDrawVisibleTilesCache = true;

    if (!m_mustDrawVisibleTilesCache)
        return;

    m_cachedVisibleTiles.clear();

    // draw from last floor (the lower) to first floor (the higher)
    for(int iz = m_cachedLastVisibleFloor; iz >= (m_floorFading ? m_cachedFirstFadingFloor : m_cachedFirstVisibleFloor); --iz) {
        // loop through / diagonals beginning at top left and going to top right
        depth -= Otc::DIAGONAL_DEPTH * 8;
        for (int diagonal = 0; diagonal < numDiagonals; ++diagonal) {
            depth -= Otc::DIAGONAL_DEPTH;

            // loop current diagonal tiles
            int advance = std::max<int>(diagonal - m_drawDimension.height(), 0);
            for (int iy = diagonal - advance, ix = advance; iy >= 0 && ix < m_drawDimension.width(); --iy, ++ix) {

                // position on current floor
                //TODO: check position limits
                Position tilePos = cameraPosition.translated(ix - m_virtualCenterOffset.x, iy - m_virtualCenterOffset.y);
                // adjust tilePos to the wanted floor
                tilePos.coveredUp(cameraPosition.z - iz);
                if (const TilePtr& tile = g_map.getTile(tilePos)) {
                    // skip tiles that have nothing
                    if (!tile->isDrawable())
                        continue;
                    // skip tiles that are completely behind another tile
                    m_cachedVisibleTiles.push_back(std::make_pair(tile, g_map.isCompletelyCovered(tilePos, m_cachedFirstVisibleFloor)));
                    tile->setDepth(depth);
                    tile->calculateTopDepth();
                }
            }
        }
        depth -= Otc::DIAGONAL_DEPTH * 8;

        m_floorDepth[iz] = depth;
    }
}

void MapView::updateGeometry(const Size& visibleDimension, const Size& optimizedSize)
{
    m_multifloor = true;
    m_visibleDimension = visibleDimension;
    m_drawDimension = visibleDimension + Size(3,3);
    m_virtualCenterOffset = (m_drawDimension/2 - Size(1,1)).toPoint();
    m_visibleCenterOffset = m_virtualCenterOffset;
    m_optimizedSize = m_drawDimension * g_sprites.spriteSize();
    m_framebuffer->resize(m_optimizedSize);
    m_mapbuffer->resize(m_optimizedSize);
    if(m_lightView)
        m_lightView->resize(m_optimizedSize);
    requestVisibleTilesCacheUpdate();
}

void MapView::onTileUpdate(const Position& pos)
{
    requestVisibleTilesCacheUpdate();
}

void MapView::onMapCenterChange(const Position& pos)
{
    requestVisibleTilesCacheUpdate();
}

void MapView::lockFirstVisibleFloor(int firstVisibleFloor)
{
    m_lockedFirstVisibleFloor = firstVisibleFloor;
    requestVisibleTilesCacheUpdate();
}

void MapView::unlockFirstVisibleFloor()
{
    m_lockedFirstVisibleFloor = -1;
    requestVisibleTilesCacheUpdate();
}

void MapView::setVisibleDimension(const Size& visibleDimension)
{
    //if(visibleDimension == m_visibleDimension)
    //    return;

    if(visibleDimension.width() % 2 != 1 || visibleDimension.height() % 2 != 1) {
        g_logger.traceError("visible dimension must be odd");
        return;
    }

    if(visibleDimension < Size(3,3)) {
        g_logger.traceError("reach max zoom in");
        return;
    }

    updateGeometry(visibleDimension, m_optimizedSize);
}

void MapView::optimizeForSize(const Size& visibleSize)
{
    updateGeometry(m_visibleDimension, visibleSize);
}

void MapView::followCreature(const CreaturePtr& creature)
{
    m_follow = true;
    m_followingCreature = creature;
    requestVisibleTilesCacheUpdate();
}

void MapView::setCameraPosition(const Position& pos)
{
    m_follow = false;
    m_customCameraPosition = pos;
    requestVisibleTilesCacheUpdate();
}

Position MapView::getPosition(const Point& point, const Size& mapSize)
{
    Position cameraPosition = getCameraPosition();

    // if we have no camera, its impossible to get the tile
    if(!cameraPosition.isValid())
        return Position();

    Rect srcRect = calcFramebufferSource(mapSize);
    float sh = srcRect.width() / (float)mapSize.width();
    float sv = srcRect.height() / (float)mapSize.height();

    Point framebufferPos = Point(point.x * sh, point.y * sv);
    Point realPos = (framebufferPos + srcRect.topLeft());
    Point centerOffset = realPos / Otc::TILE_PIXELS;

    Point tilePos2D = getVisibleCenterOffset() - m_drawDimension.toPoint() + centerOffset + Point(2,2);
    if(tilePos2D.x + cameraPosition.x < 0 && tilePos2D.y + cameraPosition.y < 0)
        return Position();

    Position position = Position(tilePos2D.x, tilePos2D.y, 0) + cameraPosition;

    if(!position.isValid())
        return Position();

    return position;
}

Point MapView::getPositionOffset(const Point& point, const Size& mapSize)
{
    Position cameraPosition = getCameraPosition();

    // if we have no camera, its impossible to get the tile
    if (!cameraPosition.isValid())
        return Point(0, 0);

    Rect srcRect = calcFramebufferSource(mapSize);
    float sh = srcRect.width() / (float)mapSize.width();
    float sv = srcRect.height() / (float)mapSize.height();

    Point framebufferPos = Point(point.x * sh, point.y * sv);
    Point realPos = (framebufferPos + srcRect.topLeft());
    return Point(realPos.x % Otc::TILE_PIXELS, realPos.y % Otc::TILE_PIXELS);
}

void MapView::move(int x, int y)
{
    m_moveOffset.x += x;
    m_moveOffset.y += y;

    int32_t tmp = m_moveOffset.x / g_sprites.spriteSize();
    bool requestTilesUpdate = false;
    if(tmp != 0) {
        m_customCameraPosition.x += tmp;
        m_moveOffset.x %= g_sprites.spriteSize();
        requestTilesUpdate = true;
    }

    tmp = m_moveOffset.y / g_sprites.spriteSize();
    if(tmp != 0) {
        m_customCameraPosition.y += tmp;
        m_moveOffset.y %= g_sprites.spriteSize();
        requestTilesUpdate = true;
    }

    if(requestTilesUpdate)
        requestVisibleTilesCacheUpdate();
}

Rect MapView::calcFramebufferSource(const Size& destSize)
{
    float scaleFactor = g_sprites.spriteSize()/(float)Otc::TILE_PIXELS;
    Point drawOffset = ((m_drawDimension - m_visibleDimension - Size(1,1)).toPoint()/2) * g_sprites.spriteSize();
    if(isFollowingCreature())
        drawOffset += m_followingCreature->getWalkOffset() * scaleFactor;

    Size srcSize = destSize;
    Size srcVisible = m_visibleDimension * g_sprites.spriteSize();
    srcSize.scale(srcVisible, Fw::KeepAspectRatio);
    drawOffset.x += (srcVisible.width() - srcSize.width()) / 2;
    drawOffset.y += (srcVisible.height() - srcSize.height()) / 2;

    return Rect(drawOffset, srcSize);
}

int MapView::calcFirstVisibleFloor(bool forFading)
{
    int z = 7;
    // return forced first visible floor
    if(m_lockedFirstVisibleFloor != -1) {
        z = m_lockedFirstVisibleFloor;
    } else {
        Position cameraPosition = getCameraPosition();

        // this could happens if the player is not known yet
        if(cameraPosition.isValid()) {
            // avoid rendering multifloors in far views
            if(!m_multifloor) {
                z = cameraPosition.z;
            } else {
                // if nothing is limiting the view, the first visible floor is 0
                int firstFloor = 0;

                // limits to underground floors while under sea level
                if(cameraPosition.z > Otc::SEA_FLOOR)
                    firstFloor = std::max<int>(cameraPosition.z - Otc::AWARE_UNDEGROUND_FLOOR_RANGE, (int)Otc::UNDERGROUND_FLOOR);

                // loop in 3x3 tiles around the camera
                for(int ix = -1; ix <= 1 && firstFloor < cameraPosition.z && !forFading; ++ix) {
                    for(int iy = -1; iy <= 1 && firstFloor < cameraPosition.z; ++iy) {
                        Position pos = cameraPosition.translated(ix, iy);

                        // process tiles that we can look through, e.g. windows, doors
                        if((ix == 0 && iy == 0) || ((std::abs(ix) != std::abs(iy)) && g_map.isLookPossible(pos))) {
                            Position upperPos = pos;
                            Position coveredPos = pos;

                            while(coveredPos.coveredUp() && upperPos.up() && upperPos.z >= firstFloor) {
                                // check tiles physically above
                                TilePtr tile = g_map.getTile(upperPos);
                                if(tile && tile->limitsFloorsView(!g_map.isLookPossible(pos))) {
                                    firstFloor = upperPos.z + 1;
                                    break;
                                }

                                // check tiles geometrically above
                                tile = g_map.getTile(coveredPos);
                                if(tile && tile->limitsFloorsView(g_map.isLookPossible(pos))) {
                                    firstFloor = coveredPos.z + 1;
                                    break;
                                }
                            }
                        }
                    }
                }
                z = firstFloor;
            }
        }
    }

    // just ensure the that the floor is in the valid range
    z = stdext::clamp<int>(z, 0, (int)Otc::MAX_Z);
    return z;
}

int MapView::calcLastVisibleFloor()
{
    if(!m_multifloor)
        return calcFirstVisibleFloor();

    int z = 7;

    Position cameraPosition = getCameraPosition();
    // this could happens if the player is not known yet
    if(cameraPosition.isValid()) {
        // view only underground floors when below sea level
        if(cameraPosition.z > Otc::SEA_FLOOR)
            z = cameraPosition.z + Otc::AWARE_UNDEGROUND_FLOOR_RANGE;
        else
            z = Otc::SEA_FLOOR;
    }

    if(m_lockedFirstVisibleFloor != -1)
        z = std::max<int>(m_lockedFirstVisibleFloor, z);

    // just ensure the that the floor is in the valid range
    z = stdext::clamp<int>(z, 0, (int)Otc::MAX_Z);
    return z;
}

Point MapView::transformPositionTo2D(const Position& position, const Position& relativePosition) {
    return Point((m_virtualCenterOffset.x + (position.x - relativePosition.x) - (relativePosition.z - position.z)) * g_sprites.spriteSize(),
        (m_virtualCenterOffset.y + (position.y - relativePosition.y) - (relativePosition.z - position.z)) * g_sprites.spriteSize());
}


Position MapView::getCameraPosition()
{
    if (isFollowingCreature()) {
        return m_followingCreature->getPrewalkingPosition();
    }

    return m_customCameraPosition;
}

void MapView::setShader(const PainterShaderProgramPtr& shader, float fadein, float fadeout)
{
    if((m_shader == shader && m_shaderSwitchDone) || (m_nextShader == shader && !m_shaderSwitchDone))
        return;

    if(fadeout > 0.0f && m_shader) {
        m_nextShader = shader;
        m_shaderSwitchDone = false;
    } else {
        m_shader = shader;
        m_nextShader = nullptr;
        m_shaderSwitchDone = true;
    }
    m_fadeTimer.restart();
    m_fadeInTime = fadein;
    m_fadeOutTime = fadeout;
}


void MapView::setDrawLights(bool enable)
{
    if (enable) {
        if (!m_lightView) {
            m_lightView = LightViewPtr(new LightView);
            m_lightView->resize(m_optimizedSize);
        }
    } else {
        m_lightView = nullptr;
    }
}

void MapView::setCrosshair(const std::string& file)     
{
    if (file == "")
        m_crosshair = nullptr;
    else
        m_crosshair = g_textures.getTexture(file);
}

/* vim: set ts=4 sw=4 et: */
