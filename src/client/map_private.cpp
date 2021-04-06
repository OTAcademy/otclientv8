// hidden code

#include "map.h"
#include "game.h"
#include "localplayer.h"
#include "tile.h"
#include "item.h"
#include "missile.h"
#include "statictext.h"
#include "mapview.h"
#include "minimap.h"

#include <framework/core/asyncdispatcher.h>
#include <framework/core/eventdispatcher.h>
#include <framework/core/application.h>
#include <framework/util/extras.h>
#include <set>

PathFindResult_ptr Map::newFindPath(const Position& start, const Position& goal, std::shared_ptr<std::list<Node*>> visibleNodes)
{
    auto ret = std::make_shared<PathFindResult>();
    ret->start = start;
    ret->destination = goal;

    if (start == goal) {
        ret->status = Otc::PathFindResultSamePosition;
        return ret;
    }
    if (goal.z != start.z) {
        return ret;
    }

    struct LessNode {
        bool operator()(Node* a, Node* b) const
        {
            return b->totalCost < a->totalCost;
        }
    };

    std::unordered_map<Position, Node*, PositionHasher> nodes;
    std::priority_queue<Node*, std::vector<Node*>, LessNode> searchList;

    if (visibleNodes) {
        for (auto& node : *visibleNodes)
            nodes.emplace(node->pos, node);
    }

    Node* initNode = new Node{ 1, 0, start, nullptr, 0, 0 };
    nodes[start] = initNode;
    searchList.push(initNode);

    int limit = 50000;
    float distance = start.distance(goal);

    Node* dstNode = nullptr;
    while (!searchList.empty() && --limit) {
        Node* node = searchList.top();
        searchList.pop();
        if (node->pos == goal) {
            dstNode = node;
            break;
        }
        if (node->pos.distance(goal) > distance + 10000)
            continue;
        for (int i = -1; i <= 1; ++i) {
            for (int j = -1; j <= 1; ++j) {
                if (i == 0 && j == 0)
                    continue;
                Position neighbor = node->pos.translated(i, j);
                if (neighbor.x < 0 || neighbor.y < 0) continue;
                auto it = nodes.find(neighbor);
                if (it == nodes.end()) {
                    auto blockAndTile = g_minimap.threadGetTile(neighbor);
                    bool wasSeen = blockAndTile.second.hasFlag(MinimapTileWasSeen);
                    bool isNotWalkable = blockAndTile.second.hasFlag(MinimapTileNotWalkable);
                    bool isNotPathable = blockAndTile.second.hasFlag(MinimapTileNotPathable);
                    bool isEmpty = blockAndTile.second.hasFlag(MinimapTileEmpty);
                    float speed = blockAndTile.second.getSpeed();
                    if ((isNotWalkable || isNotPathable || isEmpty) && neighbor != goal) {
                        it = nodes.emplace(neighbor, nullptr).first;
                    } else {
                        if (!wasSeen)
                            speed = 2000;
                        it = nodes.emplace(neighbor, new Node{ speed, 10000000.0f, neighbor, node, node->distance + 1, wasSeen ? 0 : 1 }).first;
                    }
                }
                if (!it->second) // no way
                    continue;
                if (it->second->unseen > 50)
                    continue;

                float diagonal = ((i == 0 || j == 0) ? 1.0f : 3.0f);
                float cost = it->second->cost * diagonal;
                cost += diagonal * (50.0f * std::max<float>(5.0f, it->second->pos.distance(goal))); // heuristic
                if (node->totalCost + cost + 50 < it->second->totalCost) {
                    it->second->totalCost = node->totalCost + cost;
                    it->second->prev = node;
                    if (it->second->unseen)
                        it->second->unseen = node->unseen + 1;
                    it->second->distance = node->distance + 1;
                    searchList.push(it->second);
                }
            }
        }
    }

    if (dstNode) {
        while (dstNode && dstNode->prev) {
            if (dstNode->unseen) {
                ret->path.clear();
            } else {
                ret->path.push_back(dstNode->prev->pos.getDirectionFromPosition(dstNode->pos));
            }
            dstNode = dstNode->prev;
        }
        std::reverse(ret->path.begin(), ret->path.end());
        ret->status = Otc::PathFindResultOk;
    }
    ret->complexity = 50000 - limit;

    for (auto& node : nodes) {
        if (node.second)
            delete node.second;
    }

    return ret;
}

void Map::findPathAsync(const Position& start, const Position& goal, std::function<void(PathFindResult_ptr)> callback)
{
    auto visibleNodes = std::make_shared<std::list<Node*>>();
    for (auto& tile : getTiles(start.z)) {
        if (tile->getPosition() == start)
            continue;
        bool isNotWalkable = !tile->isWalkable(false);
        bool isNotPathable = !tile->isPathable();
        float speed = tile->getGroundSpeed();
        if ((isNotWalkable || isNotPathable) && tile->getPosition() != goal) {
            visibleNodes->push_back(new Node{ speed, 0, tile->getPosition(), nullptr, 0, 0 });
        } else {
            visibleNodes->push_back(new Node{ speed, 10000000.0f, tile->getPosition(), nullptr, 0, 0 });
        }
    }

    g_asyncDispatcher.dispatch([=] {
        auto ret = g_map.newFindPath(start, goal, visibleNodes);
        g_dispatcher.addEvent(std::bind(callback, ret));
    });
}

std::map<std::string, std::tuple<int, int, int, std::string>> Map::findEveryPath(const Position& start, int maxDistance, const std::map<std::string, std::string>& params)
{
    // using Dijkstra's algorithm
    struct LessNode {
        bool operator()(Node* a, Node* b) const
        {
            return b->totalCost < a->totalCost;
        }
    };

    if (g_extras.debugPathfinding) {
        g_logger.info(stdext::format("findEveryPath: %i %i %i - %i", start.x, start.y, start.z, maxDistance));
        for (auto& param : params) {
            g_logger.info(stdext::format("%s - %s", param.first, param.second));
        }
    }

    std::map<std::string, std::string>::const_iterator it;
    it = params.find("ignoreLastCreature");
    bool ignoreLastCreature = it != params.end() && it->second != "0" && it->second != "";
    it = params.find("ignoreCreatures");
    bool ignoreCreatures = it != params.end() && it->second != "0" && it->second != "";
    it = params.find("ignoreNonPathable");
    bool ignoreNonPathable = it != params.end() && it->second != "0" && it->second != "";
    it = params.find("ignoreNonWalkable");
    bool ignoreNonWalkable = it != params.end() && it->second != "0" && it->second != "";
    it = params.find("ignoreStairs");
    bool ignoreStairs = it != params.end() && it->second != "0" && it->second != "";
    it = params.find("ignoreCost");
    bool ignoreCost = it != params.end() && it->second != "0" && it->second != "";
    it = params.find("allowUnseen");
    bool allowUnseen = it != params.end() && it->second != "0" && it->second != "";
    it = params.find("allowOnlyVisibleTiles");
    bool allowOnlyVisibleTiles = it != params.end() && it->second != "0" && it->second != "";
    it = params.find("marginMin");
    bool hasMargin = it != params.end();
    it = params.find("marginMax");
    hasMargin = hasMargin || (it != params.end());


    Position destPos;
    it = params.find("destination");
    if (it != params.end()) {
        std::vector<int32> pos = stdext::split<int32>(it->second, ",");
        if (pos.size() == 3) {
            destPos = Position(pos[0], pos[1], pos[2]);
        }
    }

    std::map<std::string, std::tuple<int, int, int, std::string>> ret;
    std::unordered_map<Position, Node*, PositionHasher> nodes;
    std::priority_queue<Node*, std::vector<Node*>, LessNode> searchList;

    Node* initNode = new Node{ 1, 0, start, nullptr, 0, 0 };
    nodes[start] = initNode;
    searchList.push(initNode);

    while (!searchList.empty()) {
        Node* node = searchList.top();
        searchList.pop();
        ret[node->pos.toString()] = std::make_tuple(node->totalCost, node->distance,
                                                    node->prev ? node->prev->pos.getDirectionFromPosition(node->pos) : -1,
                                                    node->prev ? node->prev->pos.toString() : "");
        if (node->pos == destPos) {
            if (hasMargin) {
                maxDistance = std::min<int>(node->distance + 4, maxDistance);
            } else {
                break;
            }
        }
        if (node->distance >= maxDistance)
            continue;
        for (int i = -1; i <= 1; ++i) {
            for (int j = -1; j <= 1; ++j) {
                if (i == 0 && j == 0)
                    continue;
                Position neighbor = node->pos.translated(i, j);
                if (neighbor.x < 0 || neighbor.y < 0) continue;
                auto it = nodes.find(neighbor);
                if (it == nodes.end()) {
                    bool wasSeen = false;
                    bool hasCreature = false;
                    bool isNotWalkable = true;
                    bool isNotPathable = true;
                    int mapColor = 0;
                    int speed = 1000;
                    if (g_map.isAwareOfPosition(neighbor)) {
                        if (const TilePtr& tile = getTile(neighbor)) {
                            wasSeen = true;
                            hasCreature = tile->hasBlockingCreature();
                            isNotWalkable = !tile->isWalkable(true);
                            isNotPathable = !tile->isPathable();
                            mapColor = tile->getMinimapColorByte();
                            speed = tile->getGroundSpeed();
                        }
                    } else if (!allowOnlyVisibleTiles) {
                        const MinimapTile& mtile = g_minimap.getTile(neighbor);
                        wasSeen = mtile.hasFlag(MinimapTileWasSeen);
                        isNotWalkable = mtile.hasFlag(MinimapTileNotWalkable);
                        isNotPathable = mtile.hasFlag(MinimapTileNotPathable);
                        mapColor = mtile.color;
                        if (isNotWalkable || isNotPathable)
                            wasSeen = true;
                        speed = mtile.getSpeed();
                    }
                    bool hasStairs = isNotPathable && mapColor >= 210 && mapColor <= 213;
                    if ((!wasSeen && !allowUnseen) || (hasStairs && !ignoreStairs && neighbor != destPos) || (isNotPathable && !ignoreNonPathable && neighbor != destPos) || (isNotWalkable && !ignoreNonWalkable)) {
                        it = nodes.emplace(neighbor, nullptr).first;
                    } else if ((hasCreature && !ignoreCreatures)) {
                        it = nodes.emplace(neighbor, nullptr).first;
                        if (ignoreLastCreature) {
                            ret[neighbor.toString()] = std::make_tuple(node->totalCost + 100, node->distance + 1,
                                                                       node->pos.getDirectionFromPosition(neighbor),
                                                                       node->pos.toString());
                        }
                    } else {
                        it = nodes.emplace(neighbor, new Node{ (float)speed, 10000000.0f, neighbor, node, node->distance + 1, wasSeen ? 0 : 1 }).first;
                    }
                }

                if (!it->second) {
                    continue;
                }

                float diagonal = ((i == 0 || j == 0) ? 1.0f : 3.0f);
                float cost = it->second->cost * diagonal;
                if (ignoreCost)
                    cost = 1;
                if (node->totalCost + cost < it->second->totalCost) {
                    it->second->totalCost = node->totalCost + cost;
                    it->second->prev = node;
                    if (it->second->unseen)
                        it->second->unseen = node->unseen + 1;
                    it->second->distance = node->distance + 1;
                    searchList.push(it->second);
                }
            }
        }
    }

    for (auto& node : nodes) {
        if (node.second)
            delete node.second;
    }

    return ret;
}