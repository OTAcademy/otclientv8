#include "mapviewstore.h"
#include "map.h"
#include "item.h"
#include "tile.h"
#include "thingtypemanager.h"
#include "spritemanager.h"
#include "game.h"
#include "localplayer.h"

#include <framework/core/resourcemanager.h>
#include <framework/core/filestream.h>
#include <framework/core/binarytree.h>
#include <framework/core/logger.h>
#include <framework/graphics/image.h>
#include <framework/graphics/texture.h>
#include <zlib.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

MapViewStore g_mapview;

void MapViewStore::init()
{
}

void MapViewStore::terminate()
{
    unload();
}

void MapViewStore::unload()
{
    std::lock_guard<std::mutex> lock(m_lock);
    m_catalog.clear();
    m_sectors.clear();
    m_textureLru.clear();
    m_idLru.clear();
    m_fileData.clear();
    m_liveTiles.clear();
    m_liveSectors.clear();
    m_loaded = false;
    m_bakesThisFrame = 0;
}

void MapViewStore::beginFrame()
{
    m_bakesThisFrame = 0;
}

float MapViewStore::zoomToScale(int zoom)
{
    if(zoom < 0)
        return 1.0f / static_cast<float>(1 << std::abs(zoom));
    if(zoom > 0)
        return static_cast<float>(1 << zoom);
    return 1.0f;
}

void MapViewStore::setVisualMinScale(float scale)
{
    if(scale < 0.0f)
        scale = 0.0f;
    m_visualMinScale = scale;

    int zoom = 0;
    if(scale <= 0.0f) {
        m_visualMinZoom = -20;
        return;
    }
    if(scale >= 1.0f) {
        float s = 1.0f;
        while(s * 2.0f <= scale + 0.0001f && zoom < 16) {
            s *= 2.0f;
            ++zoom;
        }
    } else {
        float s = 1.0f;
        while(s > scale + 0.0001f && zoom > -16) {
            s *= 0.5f;
            --zoom;
        }
    }
    m_visualMinZoom = zoom;
}

void MapViewStore::setVisualMinZoom(int zoom)
{
    if(zoom > 16)
        zoom = 16;
    if(zoom < -16)
        zoom = -16;
    m_visualMinZoom = zoom;
    m_visualMinScale = zoomToScale(zoom);
}

void MapViewStore::setBakesPerFrame(int count)
{
    if(count < 1)
        count = 1;
    if(count > 32)
        count = 32;
    m_maxBakesPerFrame = count;
}

void MapViewStore::setMaxCachedSectors(int count)
{
    if(count < 8)
        count = 8;
    if(count > 4096)
        count = 4096;
    m_maxTextures = count;
}

void MapViewStore::setLiveUpdate(bool enabled)
{
    std::lock_guard<std::mutex> lock(m_lock);
    if(m_liveUpdate == enabled)
        return;
    m_liveUpdate = enabled;
    if(!enabled) {
        for(auto& it : m_sectors) {
            if(it.second)
                it.second->texture.reset();
        }
    }
}

uint64 MapViewStore::tileKey(const Position& pos) const
{
    return (static_cast<uint64>(pos.z) << 40) | (static_cast<uint64>(pos.y) << 16) | pos.x;
}

MapViewTileItems MapViewStore::collectLiveItems(const TilePtr& tile) const
{
    MapViewTileItems items;
    if(!tile)
        return items;
    for(const ThingPtr& thing : tile->getThings()) {
        if(!thing || !thing->isItem())
            continue;
        ItemPtr item = thing->static_self_cast<Item>();
        if(!item || !item->isValid() || item->getClientId() == 0)
            continue;
        items.add(item->getClientId());
    }
    return items;
}

void MapViewStore::invalidateSectorTexture(const Position& pos)
{
    auto it = m_sectors.find(sectorKey(pos));
    if(it != m_sectors.end() && it->second)
        it->second->texture.reset();
}

bool MapViewStore::hasLiveSector(const Position& pos) const
{
    return m_liveSectors.find(sectorKey(pos)) != m_liveSectors.end();
}

void MapViewStore::notifySeenTile(const Position& pos)
{
    if(!m_liveUpdate || !pos.isValid() || pos.z > Otc::MAX_Z)
        return;

    const MapViewTileItems items = collectLiveItems(g_map.getTile(pos));
    const uint64 key = tileKey(pos);

    std::lock_guard<std::mutex> lock(m_lock);
    auto it = m_liveTiles.find(key);
    if(it != m_liveTiles.end() && it->second == items)
        return;

    m_liveTiles[key] = items;
    m_liveSectors.insert(sectorKey(pos));
    invalidateSectorTexture(pos);
}

void MapViewStore::drawTileItems(const ImagePtr& image, const Position& pos, const Point& dest, const MapViewTileItems& items)
{
    for(uint8 i = 0; i < items.count; ++i) {
        ItemPtr item = Item::create(items.ids[i]);
        if(!item || !item->isValid())
            continue;
        item->setPosition(pos);
        item->drawToImage(dest, image);
    }
}

uint64 MapViewStore::sectorKey(uint16 x, uint16 y, uint8 z) const
{
    x = static_cast<uint16>(x - (x % MAPVIEW_SECTOR_SIZE));
    y = static_cast<uint16>(y - (y % MAPVIEW_SECTOR_SIZE));
    return (static_cast<uint64>(z) << 40) | (static_cast<uint64>(y) << 16) | x;
}

Position MapViewStore::getSectorOrigin(const Position& pos) const
{
    return Position(static_cast<uint16>(pos.x - (pos.x % MAPVIEW_SECTOR_SIZE)),
                    static_cast<uint16>(pos.y - (pos.y % MAPVIEW_SECTOR_SIZE)),
                    pos.z);
}

bool MapViewStore::hasSector(const Position& pos) const
{
    return m_catalog.find(sectorKey(pos)) != m_catalog.end() ||
           m_sectors.find(sectorKey(pos)) != m_sectors.end();
}

MapViewSectorPtr MapViewStore::getOrCreateSector(const Position& pos)
{
    const uint64 key = sectorKey(pos);
    auto it = m_sectors.find(key);
    if(it != m_sectors.end())
        return it->second;

    auto sector = std::make_shared<MapViewSector>();
    sector->catalog.x = static_cast<uint16>(pos.x - (pos.x % MAPVIEW_SECTOR_SIZE));
    sector->catalog.y = static_cast<uint16>(pos.y - (pos.y % MAPVIEW_SECTOR_SIZE));
    sector->catalog.z = pos.z;
    sector->idsLoaded = true;
    m_sectors[key] = sector;
    m_catalog[key] = sector->catalog;
    return sector;
}

MapViewSectorPtr MapViewStore::findSector(const Position& pos)
{
    const uint64 key = sectorKey(pos);
    auto it = m_sectors.find(key);
    if(it != m_sectors.end())
        return it->second;
    return nullptr;
}

uint16 MapViewStore::clientIdFromOtb(uint16 otbId) const
{
    if(!g_things.isValidOtbId(otbId))
        return 0;
    const uint16 clientId = g_things.getItemType(otbId)->getClientId();
    if(!g_things.isValidDatId(clientId, ThingCategoryItem))
        return 0;
    return clientId;
}

bool MapViewStore::keepItem(const ItemPtr& item) const
{
    if(!item || !item->isValid() || item->getClientId() == 0)
        return false;
    if(item->isPickupable() && !item->isGround() && !item->isGroundBorder() &&
       !item->isOnBottom() && !item->isOnTop())
        return false;
    return true;
}

void MapViewStore::addTileItems(const Position& pos, const std::vector<uint16>& ids)
{
    if(ids.empty() || !pos.isValid() || pos.z > Otc::MAX_Z)
        return;

    MapViewSectorPtr sector = getOrCreateSector(pos);
    const int lx = pos.x % MAPVIEW_SECTOR_SIZE;
    const int ly = pos.y % MAPVIEW_SECTOR_SIZE;
    MapViewTileItems& tile = sector->tiles[ly * MAPVIEW_SECTOR_SIZE + lx];
    tile.count = 0;
    for(uint16 id : ids)
        tile.add(id);

    if(tile.empty())
        return;

    if(m_writeMinimapTiles)
        updateMinimapFromIds(pos, ids);
}

void MapViewStore::updateMinimapFromIds(const Position& pos, const std::vector<uint16>& ids)
{
    MinimapTile minimapTile;
    minimapTile.color = 255;
    minimapTile.flags = MinimapTileWasSeen;
    minimapTile.speed = 10;

    bool hasGround = false;
    bool notWalkable = false;
    bool notPathable = false;
    int groundSpeed = 100;

    for(uint16 id : ids) {
        if(!g_things.isValidDatId(id, ThingCategoryItem))
            continue;
        ThingType* type = g_things.rawGetThingType(id, ThingCategoryItem);
        if(!type || type->isNull())
            continue;

        if(type->isGround()) {
            hasGround = true;
            groundSpeed = type->getGroundSpeed();
        }
        if(type->isNotWalkable())
            notWalkable = true;
        if(type->isNotPathable())
            notPathable = true;

        if(!type->isGround() && !type->isGroundBorder() && !type->isOnBottom() && !type->isOnTop())
            break;

        const uint8 c = static_cast<uint8>(type->getMinimapColor());
        if(c != 0)
            minimapTile.color = c;
    }

    if(!hasGround || notWalkable)
        minimapTile.flags |= MinimapTileNotWalkable;
    if(notPathable)
        minimapTile.flags |= MinimapTileNotPathable;
    minimapTile.speed = static_cast<uint8>(std::min<int>((int)std::ceil(groundSpeed / 10.0f), 255));
    g_minimap.setTile(pos, minimapTile);
}

void MapViewStore::addTileFromGame(const TilePtr& tile)
{
    if(!tile)
        return;

    std::vector<uint16> ids;
    for(const ThingPtr& thing : tile->getThings()) {
        if(!thing || !thing->isItem())
            continue;
        ItemPtr item = thing->static_self_cast<Item>();
        if(!keepItem(item))
            continue;
        ids.push_back(item->getClientId());
    }
    addTileItems(tile->getPosition(), ids);
}

void MapViewStore::collectItemIds(const BinaryTreePtr& node, std::vector<uint16>& ids)
{
    if(!node)
        return;

    ItemPtr item = Item::createFromOtb(node->getU16());
    item->unserializeItem(node);
    if(keepItem(item))
        ids.push_back(item->getClientId());
}

bool MapViewStore::parseOtbm(const std::string& fileName)
{
    FileStreamPtr fin = g_resources.openFile(fileName, g_game.getFeature(Otc::GameDontCacheFiles));
    if(!fin)
        stdext::throw_exception(stdext::format("Unable to load map '%s'", fileName));

    char identifier[4];
    if(fin->read(identifier, 1, 4) < 4)
        stdext::throw_exception("Could not read file identifier");

    if(memcmp(identifier, "OTBM", 4) != 0 && memcmp(identifier, "\0\0\0\0", 4) != 0)
        stdext::throw_exception(stdext::format("Invalid file identifier detected: %s", identifier));

    BinaryTreePtr root = fin->getBinaryTree();
    if(root->getU8())
        stdext::throw_exception("could not read root property!");

    uint32 headerVersion = root->getU32();
    if(headerVersion > 3)
        stdext::throw_exception(stdext::format("Unknown OTBM version detected: %u.", headerVersion));

    root->getU16(); // width
    root->getU16(); // height
    root->getU8();  // major items
    root->skip(3);
    root->getU32(); // minor items

    auto children = root->getChildren();
    if(children.empty())
        stdext::throw_exception("Could not read root data node");
    BinaryTreePtr dataNode = children[0];
    if(dataNode->getU8() != OTBM_MAP_DATA)
        stdext::throw_exception("Could not read root data node");

    while(dataNode->canRead()) {
        uint8 attribute = dataNode->getU8();
        dataNode->getString();
        if(attribute != OTBM_ATTR_DESCRIPTION && attribute != OTBM_ATTR_SPAWN_FILE &&
           attribute != OTBM_ATTR_HOUSE_FILE)
            stdext::throw_exception(stdext::format("Invalid attribute '%d'", (int)attribute));
    }

    int areas = 0;
    int tiles = 0;
    for(const BinaryTreePtr& nodeMapData : dataNode->getChildren()) {
        uint8 mapDataType = nodeMapData->getU8();
        if(mapDataType != OTBM_TILE_AREA)
            continue;

        Position basePos;
        basePos.x = nodeMapData->getU16();
        basePos.y = nodeMapData->getU16();
        basePos.z = nodeMapData->getU8();

        for(const BinaryTreePtr& nodeTile : nodeMapData->getChildren()) {
            uint8 type = nodeTile->getU8();
            if(type != OTBM_TILE && type != OTBM_HOUSETILE)
                continue;

            Position pos = basePos + nodeTile->getPoint();
            if(type == OTBM_HOUSETILE)
                nodeTile->getU32();
            std::vector<uint16> ids;

            while(nodeTile->canRead()) {
                uint8 tileAttr = nodeTile->getU8();
                switch(tileAttr) {
                    case OTBM_ATTR_TILE_FLAGS:
                        nodeTile->getU32();
                        break;
                    case OTBM_ATTR_ITEM: {
                        ItemPtr groundItem = Item::createFromOtb(nodeTile->getU16());
                        if(keepItem(groundItem))
                            ids.push_back(groundItem->getClientId());
                        break;
                    }
                    default:
                        stdext::throw_exception(stdext::format("invalid tile attribute %d at pos %s",
                                                               (int)tileAttr, stdext::to_string(pos)));
                }
            }

            for(const BinaryTreePtr& nodeItem : nodeTile->getChildren()) {
                if(nodeItem->getU8() != OTBM_ITEM)
                    continue;
                collectItemIds(nodeItem, ids);
            }

            if(!ids.empty()) {
                addTileItems(pos, ids);
                ++tiles;
            }
        }

        ++areas;
        if(areas % 50 == 0)
            g_logger.info(stdext::format("Map view: parsed %d tile areas (%d tiles)", areas, tiles));
    }

    fin->close();
    g_logger.info(stdext::format("Map view: finished OTBM parse (%d areas, %d tiles, %d sectors)",
                                 areas, tiles, (int)m_catalog.size()));
    return true;
}

bool MapViewStore::generateFromOtbm(const std::string& otbmFile, const std::string& otcm2File, const std::string& otmmFile)
{
    try {
        if(!g_things.isOtbLoaded())
            stdext::throw_exception("OTB isn't loaded yet to generate a map view.");
        if(!g_things.isDatLoaded())
            stdext::throw_exception("DAT isn't loaded yet to generate a map view.");

        unload();
        m_writeMinimapTiles = !otmmFile.empty();
        if(m_writeMinimapTiles)
            g_minimap.clean();

        if(!parseOtbm(otbmFile))
            return false;

        m_loaded = !m_catalog.empty();
        if(!save(otcm2File))
            return false;
        if(m_writeMinimapTiles)
            g_minimap.saveOtmm(otmmFile);
        m_writeMinimapTiles = true;

        g_logger.info(stdext::format("Map view generated: %s (%d sectors)", otcm2File, getSectorCount()));
        return true;
    } catch(stdext::exception& e) {
        m_writeMinimapTiles = true;
        g_logger.error(stdext::format("failed to generate map view: %s", e.what()));
        return false;
    } catch(std::exception& e) {
        m_writeMinimapTiles = true;
        g_logger.error(stdext::format("failed to generate map view: %s", e.what()));
        return false;
    }
}

bool MapViewStore::generateMapView(const std::string& otbFile, const std::string& otbmFile,
                                   const std::string& otcm2File, const std::string& otmmFile)
{
    try {
        if(!otbFile.empty())
            g_things.loadOtb(otbFile);
        return generateFromOtbm(otbmFile, otcm2File, otmmFile);
    } catch(stdext::exception& e) {
        g_logger.error(stdext::format("failed to generate map view: %s", e.what()));
        return false;
    }
}

bool MapViewStore::generateFromCurrentMap(const std::string& otcm2File, const std::string& otmmFile)
{
    try {
        unload();
        g_minimap.clean();

        for(const TilePtr& tile : g_map.getTiles())
            addTileFromGame(tile);

        m_loaded = !m_catalog.empty();
        if(!save(otcm2File))
            return false;
        if(!otmmFile.empty())
            g_minimap.saveOtmm(otmmFile);
        g_logger.info(stdext::format("Map view generated from loaded map: %s (%d sectors)", otcm2File, getSectorCount()));
        return true;
    } catch(stdext::exception& e) {
        g_logger.error(stdext::format("failed to generate map view from current map: %s", e.what()));
        return false;
    }
}

bool MapViewStore::packSector(const MapViewSector& sector, std::vector<uint8>& out) const
{
    out.clear();
    uint16 occupied = 0;
    for(const MapViewTileItems& tile : sector.tiles) {
        if(!tile.empty())
            ++occupied;
    }

    out.resize(2);
    out[0] = static_cast<uint8>(occupied & 0xFF);
    out[1] = static_cast<uint8>((occupied >> 8) & 0xFF);

    for(int i = 0; i < MAPVIEW_SECTOR_SIZE * MAPVIEW_SECTOR_SIZE; ++i) {
        const MapViewTileItems& tile = sector.tiles[i];
        if(tile.empty())
            continue;
        const uint8 lx = static_cast<uint8>(i % MAPVIEW_SECTOR_SIZE);
        const uint8 ly = static_cast<uint8>(i / MAPVIEW_SECTOR_SIZE);
        out.push_back(lx);
        out.push_back(ly);
        out.push_back(tile.count);
        for(uint8 n = 0; n < tile.count; ++n) {
            out.push_back(static_cast<uint8>(tile.ids[n] & 0xFF));
            out.push_back(static_cast<uint8>((tile.ids[n] >> 8) & 0xFF));
        }
    }
    return occupied > 0;
}

bool MapViewStore::unpackSector(MapViewSector& sector, const uint8* data, size_t size) const
{
    if(size < 2)
        return false;
    size_t p = 0;
    uint16 occupied = static_cast<uint16>(data[p] | (data[p + 1] << 8));
    p += 2;
    for(uint16 i = 0; i < occupied; ++i) {
        if(p + 3 > size)
            return false;
        const uint8 lx = data[p++];
        const uint8 ly = data[p++];
        const uint8 n = data[p++];
        if(lx >= MAPVIEW_SECTOR_SIZE || ly >= MAPVIEW_SECTOR_SIZE)
            return false;
        if(p + n * 2 > size)
            return false;
        MapViewTileItems& tile = sector.tiles[ly * MAPVIEW_SECTOR_SIZE + lx];
        tile.count = 0;
        for(uint8 k = 0; k < n; ++k) {
            uint16 id = static_cast<uint16>(data[p] | (data[p + 1] << 8));
            p += 2;
            tile.add(id);
        }
    }
    sector.idsLoaded = true;
    return true;
}

bool MapViewStore::isNearLoadedMap(const Position& pos) const
{
    const LocalPlayerPtr player = g_game.getLocalPlayer();
    if(!player)
        return false;
    const Position p = player->getPosition();
    if(!p.isValid() || p.z != pos.z)
        return false;
    const int ox = static_cast<int>(pos.x - (pos.x % MAPVIEW_SECTOR_SIZE));
    const int oy = static_cast<int>(pos.y - (pos.y % MAPVIEW_SECTOR_SIZE));
    const int dx = static_cast<int>(p.x) - ox;
    const int dy = static_cast<int>(p.y) - oy;
    return dx >= -18 && dx < MAPVIEW_SECTOR_SIZE + 18 &&
           dy >= -18 && dy < MAPVIEW_SECTOR_SIZE + 18;
}

int MapViewStore::lowestVisibleFloor(int z) const
{
    return (z <= 7) ? 7 : Otc::MAX_Z;
}

bool MapViewStore::hasLowerFloorData(const Position& pos) const
{
    const int z = pos.z;
    const int lastFloor = lowestVisibleFloor(z);
    if(lastFloor <= z)
        return false;

    const uint16 x = static_cast<uint16>(pos.x - (pos.x % MAPVIEW_SECTOR_SIZE));
    const uint16 y = static_cast<uint16>(pos.y - (pos.y % MAPVIEW_SECTOR_SIZE));
    for(int floor = z + 1; floor <= lastFloor; ++floor) {
        const uint64 key = sectorKey(x, y, static_cast<uint8>(floor));
        if(m_catalog.find(key) != m_catalog.end())
            return true;
        if(m_liveUpdate && m_liveSectors.find(key) != m_liveSectors.end())
            return true;
        auto it = m_sectors.find(key);
        if(it == m_sectors.end() || !it->second || !it->second->idsLoaded)
            continue;
        for(const MapViewTileItems& tile : it->second->tiles) {
            if(!tile.empty())
                return true;
        }
    }
    return false;
}

bool MapViewStore::fillSectorFromGame(MapViewSector& sector)
{
    bool any = false;
    Position pos;
    pos.z = sector.catalog.z;
    for(int ly = 0; ly < MAPVIEW_SECTOR_SIZE; ++ly) {
        for(int lx = 0; lx < MAPVIEW_SECTOR_SIZE; ++lx) {
            pos.x = static_cast<uint16>(sector.catalog.x + lx);
            pos.y = static_cast<uint16>(sector.catalog.y + ly);
            const TilePtr tile = g_map.getTile(pos);
            if(!tile)
                continue;

            MapViewTileItems& dst = sector.tiles[ly * MAPVIEW_SECTOR_SIZE + lx];
            dst.count = 0;
            for(const ThingPtr& thing : tile->getThings()) {
                if(!thing || !thing->isItem())
                    continue;
                ItemPtr item = thing->static_self_cast<Item>();
                if(!keepItem(item))
                    continue;
                dst.add(item->getClientId());
            }
            if(!dst.empty())
                any = true;
        }
    }
    sector.idsLoaded = true;
    return any;
}

bool MapViewStore::save(const std::string& fileName)
{
    try {
        std::lock_guard<std::mutex> lock(m_lock);
        FileStreamPtr fin = g_resources.createFile(fileName);
        if(!fin)
            stdext::throw_exception(stdext::format("failed to open file '%s' for write", fileName));

        std::vector<std::pair<uint64, MapViewSectorPtr>> sectors;
        sectors.reserve(m_sectors.size());
        for(auto& it : m_sectors) {
            if(!it.second)
                continue;
            bool empty = true;
            for(const MapViewTileItems& tile : it.second->tiles) {
                if(!tile.empty()) {
                    empty = false;
                    break;
                }
            }
            if(!empty)
                sectors.push_back(it);
        }

        std::sort(sectors.begin(), sectors.end(), [](const auto& a, const auto& b) {
            if(a.second->catalog.z != b.second->catalog.z)
                return a.second->catalog.z < b.second->catalog.z;
            if(a.second->catalog.y != b.second->catalog.y)
                return a.second->catalog.y < b.second->catalog.y;
            return a.second->catalog.x < b.second->catalog.x;
        });

        const uint32 sectorCount = static_cast<uint32>(sectors.size());
        const uint32 headerSize = 22;
        const uint32 catalogBytes = sectorCount * 18;
        const uint32 dataStart = headerSize + catalogBytes;

        fin->addU32(OTCM2_SIGNATURE);
        fin->addU16(OTCM2_VERSION);
        fin->addU16(MAPVIEW_SECTOR_SIZE);
        fin->addU8(MAPVIEW_TILE_PX);
        fin->addU8(0);
        fin->addU32(g_things.getDatSignature());
        fin->addU32(g_sprites.getSignature());
        fin->addU32(sectorCount);

        std::vector<uint8> catalog(catalogBytes, 0);
        fin->write(catalog.data(), catalog.size());

        uint32 writeOffset = dataStart;
        std::vector<uint8> packed;
        std::vector<uint8> compressed;
        const int COMPRESS_LEVEL = 6;

        for(size_t i = 0; i < sectors.size(); ++i) {
            MapViewSector& sector = *sectors[i].second;
            packSector(sector, packed);
            ulong bound = compressBound(static_cast<ulong>(packed.size()));
            compressed.resize(bound);
            ulong destLen = bound;
            int ret = compress2(compressed.data(), &destLen, packed.data(), static_cast<ulong>(packed.size()), COMPRESS_LEVEL);
            if(ret != Z_OK)
                stdext::throw_exception("zlib compress failed");

            sector.catalog.offset = writeOffset;
            sector.catalog.packedSize = static_cast<uint32>(destLen);
            sector.catalog.unpackedSize = static_cast<uint32>(packed.size());
            m_catalog[sectors[i].first] = sector.catalog;

            fin->write(compressed.data(), destLen);
            writeOffset += static_cast<uint32>(destLen);

            const size_t catOff = i * 18;
            catalog[catOff + 0] = static_cast<uint8>(sector.catalog.x & 0xFF);
            catalog[catOff + 1] = static_cast<uint8>((sector.catalog.x >> 8) & 0xFF);
            catalog[catOff + 2] = static_cast<uint8>(sector.catalog.y & 0xFF);
            catalog[catOff + 3] = static_cast<uint8>((sector.catalog.y >> 8) & 0xFF);
            catalog[catOff + 4] = sector.catalog.z;
            catalog[catOff + 5] = 0;
            memcpy(&catalog[catOff + 6], &sector.catalog.offset, 4);
            memcpy(&catalog[catOff + 10], &sector.catalog.packedSize, 4);
            memcpy(&catalog[catOff + 14], &sector.catalog.unpackedSize, 4);
        }

        fin->seek(headerSize);
        fin->write(catalog.data(), catalog.size());
        fin->flush();
        fin->close();
        m_loaded = true;
        return true;
    } catch(stdext::exception& e) {
        g_logger.error(stdext::format("failed to save OTCM2: %s", e.what()));
        return false;
    }
}

bool MapViewStore::load(const std::string& fileName)
{
    try {
        unload();
        FileStreamPtr fin = g_resources.openFile(fileName, g_game.getFeature(Otc::GameDontCacheFiles));
        if(!fin)
            stdext::throw_exception("unable to open file");

        const uint fileSize = fin->size();
        if(fileSize < 22)
            stdext::throw_exception("file too small");

        m_fileData.resize(fileSize);
        if(fin->read(m_fileData.data(), 1, fileSize) != (int)fileSize)
            stdext::throw_exception("unable to read OTCM2 file");
        fin->close();

        auto readU16 = [&](size_t off) -> uint16 {
            return static_cast<uint16>(m_fileData[off] | (m_fileData[off + 1] << 8));
        };
        auto readU32 = [&](size_t off) -> uint32 {
            uint32 v;
            memcpy(&v, &m_fileData[off], 4);
            return v;
        };

        const uint32 signature = readU32(0);
        if(signature != OTCM2_SIGNATURE)
            stdext::throw_exception("invalid OTCM2 file");
        const uint16 version = readU16(4);
        if(version != OTCM2_VERSION)
            stdext::throw_exception("OTCM2 version not supported");
        const uint16 sectorSize = readU16(6);
        if(sectorSize != MAPVIEW_SECTOR_SIZE)
            stdext::throw_exception("unsupported OTCM2 sector size");

        const uint8 tilePx = m_fileData[8];
        if(tilePx != MAPVIEW_TILE_PX)
            g_logger.warning(stdext::format("map view tile size is %d, expected %d", (int)tilePx, MAPVIEW_TILE_PX));

        const uint32 datSignature = readU32(10);
        if(g_things.isDatLoaded() && datSignature != g_things.getDatSignature())
            g_logger.warning("map view was created with a different dat signature");

        const uint32 sectorCount = readU32(18);
        const size_t catalogBytes = static_cast<size_t>(sectorCount) * 18;
        if(22 + catalogBytes > m_fileData.size())
            stdext::throw_exception("truncated OTCM2 catalog");

        size_t off = 22;
        for(uint32 i = 0; i < sectorCount; ++i) {
            MapViewCatalogEntry entry;
            entry.x = readU16(off);
            entry.y = readU16(off + 2);
            entry.z = m_fileData[off + 4];
            entry.offset = readU32(off + 6);
            entry.packedSize = readU32(off + 10);
            entry.unpackedSize = readU32(off + 14);
            if(entry.offset + entry.packedSize > m_fileData.size())
                stdext::throw_exception("truncated OTCM2 sector blob");
            m_catalog[sectorKey(entry.x, entry.y, entry.z)] = entry;
            off += 18;
        }

        m_loaded = !m_catalog.empty();
        g_logger.info(stdext::format("Loaded map view '%s' (%d sectors)", fileName, getSectorCount()));
        return m_loaded;
    } catch(stdext::exception& e) {
        g_logger.error(stdext::format("failed to load OTCM2: %s", e.what()));
        unload();
        return false;
    }
}

bool MapViewStore::loadSectorIds(MapViewSector& sector)
{
    if(sector.idsLoaded)
        return true;
    if(sector.catalog.packedSize == 0 || sector.catalog.offset + sector.catalog.packedSize > m_fileData.size())
        return false;

    std::vector<uint8> unpacked(sector.catalog.unpackedSize);
    ulong destLen = sector.catalog.unpackedSize;
    int ret = uncompress(unpacked.data(), &destLen,
                         m_fileData.data() + sector.catalog.offset,
                         sector.catalog.packedSize);
    if(ret != Z_OK || destLen != sector.catalog.unpackedSize)
        return false;
    return unpackSector(sector, unpacked.data(), unpacked.size());
}

void MapViewStore::touchTexture(uint64 key)
{
    m_textureLru.remove(key);
    m_textureLru.push_front(key);
}

void MapViewStore::evictTextures()
{
    while(static_cast<int>(m_textureLru.size()) > m_maxTextures) {
        uint64 oldKey = m_textureLru.back();
        m_textureLru.pop_back();
        auto it = m_sectors.find(oldKey);
        if(it != m_sectors.end() && it->second)
            it->second->texture.reset();
    }
}

void MapViewStore::evictIdCache()
{
    if(m_fileData.empty())
        return;
    while(m_idLru.size() > MAPVIEW_ID_CACHE) {
        uint64 oldKey = m_idLru.back();
        m_idLru.pop_back();
        auto it = m_sectors.find(oldKey);
        if(it != m_sectors.end() && it->second && !it->second->texture) {
            it->second->tiles = {};
            it->second->idsLoaded = false;
        }
    }
}

void MapViewStore::drawItemsToImage(const ImagePtr& image, const Position& pos, const Point& dest)
{
    if(m_liveUpdate) {
        auto liveIt = m_liveTiles.find(tileKey(pos));
        if(liveIt != m_liveTiles.end()) {
            drawTileItems(image, pos, dest, liveIt->second);
            return;
        }
    }

    const uint64 key = sectorKey(pos);
    MapViewSectorPtr sector = findSector(pos);
    if(!sector) {
        auto catIt = m_catalog.find(key);
        if(catIt != m_catalog.end()) {
            sector = std::make_shared<MapViewSector>();
            sector->catalog = catIt->second;
            if(loadSectorIds(*sector))
                m_sectors[key] = sector;
            else
                sector.reset();
        }
    } else if(!sector->idsLoaded) {
        if(!loadSectorIds(*sector))
            sector.reset();
    }

    if(sector) {
        const int lx = pos.x % MAPVIEW_SECTOR_SIZE;
        const int ly = pos.y % MAPVIEW_SECTOR_SIZE;
        const MapViewTileItems& tile = sector->tiles[ly * MAPVIEW_SECTOR_SIZE + lx];
        if(!tile.empty()) {
            drawTileItems(image, pos, dest, tile);
            return;
        }
    }

    const TilePtr gameTile = g_map.getTile(pos);
    if(gameTile)
        gameTile->drawToImage(dest, image);
}

ImagePtr MapViewStore::cropImage(const ImagePtr& src, int x, int y, int w, int h) const
{
    auto out = std::make_shared<Image>(Size(w, h), src->getBpp());
    const int sw = src->getWidth();
    const int sh = src->getHeight();
    const int bpp = src->getBpp();
    for(int yy = 0; yy < h; ++yy) {
        const int sy = y + yy;
        if(sy < 0 || sy >= sh)
            continue;
        for(int xx = 0; xx < w; ++xx) {
            const int sx = x + xx;
            if(sx < 0 || sx >= sw)
                continue;
            memcpy(out->getPixel(xx, yy), src->getPixel(sx, sy), bpp);
        }
    }
    return out;
}

ImagePtr MapViewStore::downscaleImage(const ImagePtr& src, int dstW, int dstH) const
{
    auto out = std::make_shared<Image>(Size(dstW, dstH), src->getBpp());
    const int sw = src->getWidth();
    const int sh = src->getHeight();
    if(sw <= 0 || sh <= 0)
        return out;

    for(int y = 0; y < dstH; ++y) {
        const int y0 = y * sh / dstH;
        const int y1 = std::max(y0 + 1, (y + 1) * sh / dstH);
        for(int x = 0; x < dstW; ++x) {
            const int x0 = x * sw / dstW;
            const int x1 = std::max(x0 + 1, (x + 1) * sw / dstW);
            int r = 0, g = 0, b = 0, a = 0, used = 0;
            for(int sy = y0; sy < y1 && sy < sh; ++sy) {
                for(int sx = x0; sx < x1 && sx < sw; ++sx) {
                    uint8* p = src->getPixel(sx, sy);
                    if(p[3] < 16)
                        continue;
                    r += p[0];
                    g += p[1];
                    b += p[2];
                    a += p[3];
                    ++used;
                }
            }
            if(used > 0) {
                uint8 pix[4] = {
                    static_cast<uint8>(r / used),
                    static_cast<uint8>(g / used),
                    static_cast<uint8>(b / used),
                    static_cast<uint8>(a / used)
                };
                out->setPixel(x, y, pix);
            }
        }
    }
    return out;
}

void MapViewStore::bakeSector(MapViewSector& sector)
{
    if(!loadSectorIds(sector))
        sector.idsLoaded = true;

    const int maxLowerOffset = 8;
    const int pad = maxLowerOffset;
    const int spriteSize = g_sprites.spriteSize();
    if(spriteSize <= 0)
        return;
    const int tiles = MAPVIEW_SECTOR_SIZE + pad * 2 + maxLowerOffset;
    auto image = std::make_shared<Image>(Size(tiles * spriteSize, tiles * spriteSize));
    const Position origin(sector.catalog.x, sector.catalog.y, sector.catalog.z);

    auto drawFloor = [&](int floor, int extraOffset) {
        Position pos;
        pos.z = static_cast<uint8>(floor);
        for(int ly = -pad; ly < MAPVIEW_SECTOR_SIZE + pad; ++ly) {
            for(int lx = -pad; lx < MAPVIEW_SECTOR_SIZE + pad; ++lx) {
                const int px = origin.x + lx;
                const int py = origin.y + ly;
                if(px < 0 || py < 0 || px > 65535 || py > 65535)
                    continue;
                pos.x = static_cast<uint16>(px);
                pos.y = static_cast<uint16>(py);
                const int destX = (lx + pad + extraOffset) * spriteSize;
                const int destY = (ly + pad + extraOffset) * spriteSize;
                if(destX < 0 || destY < 0)
                    continue;
                drawItemsToImage(image, pos, Point(destX, destY));
            }
        }
    };

    const int z = sector.catalog.z;
    const int lastFloor = lowestVisibleFloor(z);
    if(lastFloor > z) {
        for(int floor = lastFloor; floor > z; --floor)
            drawFloor(floor, floor - z);
        image->addShadow(70);
    }
    drawFloor(z, 0);

    const int cropOrigin = pad * spriteSize;
    const int cropSize = MAPVIEW_SECTOR_SIZE * spriteSize;
    ImagePtr cropped = cropImage(image, cropOrigin, cropOrigin, cropSize, cropSize);
    const int outSize = MAPVIEW_SECTOR_SIZE * MAPVIEW_TILE_PX;
    ImagePtr scaled = downscaleImage(cropped, outSize, outSize);

    sector.texture = std::make_shared<Texture>(scaled, false, false, false);
    sector.texture->setSmooth(false);
}

TexturePtr MapViewStore::getSectorTexture(const Position& pos)
{
    std::lock_guard<std::mutex> lock(m_lock);
    if(!pos.isValid() || pos.z > Otc::MAX_Z)
        return nullptr;
    if(!m_loaded && !g_game.getLocalPlayer())
        return nullptr;

    const uint64 key = sectorKey(pos);
    auto catIt = m_catalog.find(key);

    MapViewSectorPtr sector = findSector(pos);
    if(sector && sector->texture) {
        touchTexture(key);
        return sector->texture;
    }

    if(m_bakesThisFrame >= m_maxBakesPerFrame)
        return nullptr;

    const bool inCatalog = catIt != m_catalog.end();
    const bool lowerFloors = !inCatalog && hasLowerFloorData(pos);
    const bool nearLive = !inCatalog && isNearLoadedMap(pos);
    const bool seenLive = !inCatalog && m_liveUpdate && hasLiveSector(pos);
    if(!inCatalog && !lowerFloors && !nearLive && !seenLive)
        return nullptr;

    if(!sector) {
        sector = std::make_shared<MapViewSector>();
        if(inCatalog) {
            sector->catalog = catIt->second;
        } else {
            sector->catalog.x = static_cast<uint16>(pos.x - (pos.x % MAPVIEW_SECTOR_SIZE));
            sector->catalog.y = static_cast<uint16>(pos.y - (pos.y % MAPVIEW_SECTOR_SIZE));
            sector->catalog.z = pos.z;
            sector->idsLoaded = true;
            if(nearLive)
                fillSectorFromGame(*sector);
        }
        m_sectors[key] = sector;
    } else if(!inCatalog && nearLive && !sector->idsLoaded) {
        fillSectorFromGame(*sector);
    }

    ++m_bakesThisFrame;
    bakeSector(*sector);
    if(!sector->texture)
        return nullptr;

    m_idLru.remove(key);
    m_idLru.push_front(key);
    touchTexture(key);
    evictTextures();
    evictIdCache();
    return sector->texture;
}
