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

#ifndef MAPVIEW_H
#define MAPVIEW_H

#include "declarations.h"
#include <framework/graphics/paintershaderprogram.h>
#include <framework/graphics/declarations.h>
#include <framework/luaengine/luaobject.h>
#include <framework/core/declarations.h>
#include "lightview.h"

// @bindclass
class MapView : public LuaObject
{
public:
    MapView();
    ~MapView();
    void draw(const Rect& rect, const TilePtr& crosshairTile = nullptr);
    void drawTiles(bool map, bool creatures, bool isFading, const TilePtr& crosshairTile);

private:
    void updateGeometry(const Size& visibleDimension, const Size& optimizedSize);
    void updateVisibleTilesCache();
    void requestVisibleTilesCacheUpdate() { m_mustUpdateVisibleTilesCache = true; }

protected:
    void onTileUpdate(const Position& pos);
    void onMapCenterChange(const Position& pos);

    friend class Map;

public:
    // floor visibility related
    void lockFirstVisibleFloor(int firstVisibleFloor);
    void unlockFirstVisibleFloor();
    int getLockedFirstVisibleFloor() { return m_lockedFirstVisibleFloor; }

    void setMultifloor(bool enable) { m_multifloor = enable; requestVisibleTilesCacheUpdate(); }
    bool isMultifloor() { return m_multifloor; }

    // map dimension related
    void setVisibleDimension(const Size& visibleDimension);
    void optimizeForSize(const Size & visibleSize);
    Size getVisibleDimension() { return m_visibleDimension; }
    int getTileSize() { return m_tileSize; }
    Point getVisibleCenterOffset() { return m_visibleCenterOffset; }
    int getCachedFirstVisibleFloor() { return m_cachedFirstVisibleFloor; }
    int getCachedLastVisibleFloor() { return m_cachedLastVisibleFloor; }

    // camera related
    void followCreature(const CreaturePtr& creature);
    CreaturePtr getFollowingCreature() { return m_followingCreature; }
    bool isFollowingCreature() { return m_followingCreature && m_follow; }

    void setCameraPosition(const Position& pos);
    Position getCameraPosition();

    void setMinimumAmbientLight(float intensity) { m_minimumAmbientLight = intensity; }
    float getMinimumAmbientLight() { return m_minimumAmbientLight; }

    // drawing related
    void setDrawFlags(Otc::DrawFlags drawFlags) { m_drawFlags = drawFlags; requestVisibleTilesCacheUpdate(); }
    Otc::DrawFlags getDrawFlags() { return m_drawFlags; }

    void setDrawTexts(bool enable) { m_drawTexts = enable; }
    bool isDrawingTexts() { return m_drawTexts; }

    void setDrawNames(bool enable) { m_drawNames = enable; }
    bool isDrawingNames() { return m_drawNames; }

    void setDrawHealthBars(bool enable) { m_drawHealthBars = enable; }
    bool isDrawingHealthBars() { return m_drawHealthBars; }

    void setDrawLights(bool enable);
    bool isDrawingLights() { return m_lightView != nullptr; }

    void setDrawManaBar(bool enable) { m_drawManaBar = enable; }
    bool isDrawingManaBar() { return m_drawManaBar; }

    void setDrawPlayerBars(bool enable) { m_drawPlayerBars = enable; }

    void move(int x, int y);

    void setAnimated(bool animated) { m_animated = animated; requestVisibleTilesCacheUpdate(); }
    bool isAnimating() { return m_animated; }

    void setFloorFading(int value) { m_floorFading = value; }
    void setCrosshair(const std::string& file);

    void setAddLightMethod(bool add) { m_lightView->setBlendEquation(add ? Painter::BlendEquation_Add : Painter::BlendEquation_Max); }

    void setShader(const PainterShaderProgramPtr& shader, float fadein, float fadeout);
    PainterShaderProgramPtr getShader() { return m_shader; }

    Position getPosition(const Point& point, const Size& mapSize);

    MapViewPtr asMapView() { return static_self_cast<MapView>(); }

private:
    Rect calcFramebufferSource(const Size& destSize);
    int calcFirstVisibleFloor(bool forFading = false);
    int calcLastVisibleFloor();
    Point transformPositionTo2D(const Position& position, const Position& relativePosition) {
        return Point((m_virtualCenterOffset.x + (position.x - relativePosition.x) - (relativePosition.z - position.z)) * m_tileSize,
                     (m_virtualCenterOffset.y + (position.y - relativePosition.y) - (relativePosition.z - position.z)) * m_tileSize);
    }

    stdext::timer m_mapRenderTimer;
    stdext::timer m_creaturesRenderTimer;

    int m_lockedFirstVisibleFloor;
    int m_cachedFirstVisibleFloor;
    int m_cachedFirstFadingFloor;
    int m_cachedLastVisibleFloor;
    int m_tileSize;
    int m_updateTilesPos;
    int m_floorFading = 500;
    TexturePtr m_crosshair = nullptr;
    Size m_drawDimension;
    Size m_visibleDimension;
    Size m_optimizedSize;
    Point m_virtualCenterOffset;
    Point m_visibleCenterOffset;
    Point m_moveOffset;
    Position m_customCameraPosition;
    Position m_lastCameraPosition;
    stdext::boolean<true> m_mustUpdateVisibleTilesCache;
    stdext::boolean<true> m_mustDrawVisibleTilesCache;
    stdext::boolean<true> m_multifloor;
    stdext::boolean<true> m_animated;
    stdext::boolean<true> m_drawTexts;
    stdext::boolean<true> m_drawNames;
    stdext::boolean<true> m_drawHealthBars;
    stdext::boolean<true> m_drawManaBar;
    bool m_drawPlayerBars = true;
    stdext::boolean<true> m_smooth;


    stdext::timer m_fadingFloorTimers[Otc::MAX_Z + 1];

    stdext::boolean<true> m_follow;
    std::vector<std::pair<TilePtr, bool>> m_cachedVisibleTiles;
    std::vector<CreaturePtr> m_cachedFloorVisibleCreatures;
    CreaturePtr m_followingCreature;
    FrameBufferPtr m_framebuffer;
    FrameBufferPtr m_mapbuffer;
    PainterShaderProgramPtr m_shader;
    Otc::DrawFlags m_drawFlags;
    std::vector<Point> m_spiral;
    LightViewPtr m_lightView;
    float m_minimumAmbientLight;
    Timer m_fadeTimer;
    PainterShaderProgramPtr m_nextShader;
    float m_fadeInTime;
    float m_fadeOutTime;
    stdext::boolean<true> m_shaderSwitchDone;
};

#endif
