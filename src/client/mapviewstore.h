#ifndef MAPVIEWSTORE_H
#define MAPVIEWSTORE_H

#include "declarations.h"
#include "position.h"
#include "minimap.h"

#include <framework/core/declarations.h>
#include <framework/graphics/declarations.h>

#include <array>
#include <list>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

enum {
    MAPVIEW_SECTOR_SIZE = 32,
    MAPVIEW_TILE_PX = 16,
    MAPVIEW_MAX_ITEMS = 12,
    MAPVIEW_MAX_TEXTURES = 48,
    MAPVIEW_BAKES_PER_FRAME = 2,
    MAPVIEW_ID_CACHE = 128,
    OTCM2_SIGNATURE = 0x324D4354,
    OTCM2_VERSION = 1
};

struct MapViewTileItems
{
    uint8 count = 0;
    uint16 ids[MAPVIEW_MAX_ITEMS];

    void add(uint16 id)
    {
        if(id != 0 && count < MAPVIEW_MAX_ITEMS)
            ids[count++] = id;
    }

    bool empty() const { return count == 0; }
    bool operator==(const MapViewTileItems& other) const
    {
        if(count != other.count)
            return false;
        for(uint8 i = 0; i < count; ++i) {
            if(ids[i] != other.ids[i])
                return false;
        }
        return true;
    }
};

struct MapViewCatalogEntry
{
    uint16 x = 0;
    uint16 y = 0;
    uint8 z = 0;
    uint32 offset = 0;
    uint32 packedSize = 0;
    uint32 unpackedSize = 0;
};

struct MapViewSector
{
    MapViewCatalogEntry catalog;
    std::array<MapViewTileItems, MAPVIEW_SECTOR_SIZE * MAPVIEW_SECTOR_SIZE> tiles;
    TexturePtr texture;
    bool idsLoaded = false;
};

using MapViewSectorPtr = std::shared_ptr<MapViewSector>;

class MapViewStore
{
public:
    void init();
    void terminate();
    void unload();

    bool load(const std::string& fileName);
    bool save(const std::string& fileName);
    bool isLoaded() { return m_loaded; }
    int getSectorCount() { return static_cast<int>(m_catalog.size()); }

    bool generateMapView(const std::string& otbFile, const std::string& otbmFile,
                         const std::string& otcm2File, const std::string& otmmFile);
    bool generateFromOtbm(const std::string& otbmFile, const std::string& otcm2File, const std::string& otmmFile);
    bool generateFromCurrentMap(const std::string& otcm2File, const std::string& otmmFile);

    TexturePtr getSectorTexture(const Position& pos);
    bool hasSector(const Position& pos) const;
    Position getSectorOrigin(const Position& pos) const;
    void beginFrame();

    void setVisualMinScale(float scale);
    float getVisualMinScale() { return m_visualMinScale; }
    void setVisualMinZoom(int zoom);
    int getVisualMinZoom() { return m_visualMinZoom; }
    void setBakesPerFrame(int count);
    int getBakesPerFrame() { return m_maxBakesPerFrame; }
    void setMaxCachedSectors(int count);
    int getMaxCachedSectors() { return m_maxTextures; }

    void setLiveUpdate(bool enabled);
    bool isLiveUpdate() { return m_liveUpdate; }
    void notifySeenTile(const Position& pos);

    static float zoomToScale(int zoom);

private:
    uint64 sectorKey(uint16 x, uint16 y, uint8 z) const;
    uint64 sectorKey(const Position& pos) const { return sectorKey(pos.x, pos.y, pos.z); }
    uint64 tileKey(const Position& pos) const;
    MapViewSectorPtr getOrCreateSector(const Position& pos);
    MapViewSectorPtr findSector(const Position& pos);
    bool loadSectorIds(MapViewSector& sector);
    void bakeSector(MapViewSector& sector);
    void touchTexture(uint64 key);
    void evictTextures();
    void evictIdCache();
    void invalidateSectorTexture(const Position& pos);
    MapViewTileItems collectLiveItems(const TilePtr& tile) const;
    void drawTileItems(const ImagePtr& image, const Position& pos, const Point& dest, const MapViewTileItems& items);
    bool hasLiveSector(const Position& pos) const;

    void addTileItems(const Position& pos, const std::vector<uint16>& ids);
    void addTileFromGame(const TilePtr& tile);
    void updateMinimapFromIds(const Position& pos, const std::vector<uint16>& ids);
    bool keepItem(const ItemPtr& item) const;
    uint16 clientIdFromOtb(uint16 otbId) const;
    bool parseOtbm(const std::string& fileName);
    void collectItemIds(const BinaryTreePtr& node, std::vector<uint16>& ids);
    bool packSector(const MapViewSector& sector, std::vector<uint8>& out) const;
    bool unpackSector(MapViewSector& sector, const uint8* data, size_t size) const;
    bool fillSectorFromGame(MapViewSector& sector);
    bool isNearLoadedMap(const Position& pos) const;
    bool hasLowerFloorData(const Position& pos) const;
    int lowestVisibleFloor(int z) const;
    void drawItemsToImage(const ImagePtr& image, const Position& pos, const Point& dest);
    ImagePtr cropImage(const ImagePtr& src, int x, int y, int w, int h) const;
    ImagePtr downscaleImage(const ImagePtr& src, int dstW, int dstH) const;

    std::unordered_map<uint64, MapViewCatalogEntry> m_catalog;
    std::unordered_map<uint64, MapViewSectorPtr> m_sectors;
    std::list<uint64> m_textureLru;
    std::list<uint64> m_idLru;
    std::vector<uint8> m_fileData;
    std::mutex m_lock;
    bool m_loaded = false;
    int m_bakesThisFrame = 0;
    float m_visualMinScale = 8.0f;
    int m_visualMinZoom = 3;
    int m_maxBakesPerFrame = MAPVIEW_BAKES_PER_FRAME;
    int m_maxTextures = MAPVIEW_MAX_TEXTURES;
    bool m_writeMinimapTiles = true;
    bool m_liveUpdate = false;
    std::unordered_map<uint64, MapViewTileItems> m_liveTiles;
    std::unordered_set<uint64> m_liveSectors;
};

extern MapViewStore g_mapview;

#endif
