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

#ifndef CLIENT_DECLARATIONS_H
#define CLIENT_DECLARATIONS_H

#include "global.h"
#include <framework/net/declarations.h>
#include <framework/ui/declarations.h>

// core
class Map;
class Game;
class MapView;
class LightView;
class Tile;
class Thing;
class Item;
class Container;
class Creature;
class Monster;
class Npc;
class Player;
class LocalPlayer;
class Effect;
class Missile;
class AnimatedText;
class StaticText;
class Animator;
class ThingType;
class ItemType;
class House;
class Town;
class CreatureType;
class Spawn;
class TileBlock;

using MapViewPtr = std::shared_ptr<MapView>;
using LightViewPtr = std::shared_ptr<LightView>;
using TilePtr = std::shared_ptr<Tile>;
using ThingPtr = std::shared_ptr<Thing>;
using ItemPtr = std::shared_ptr<Item>;
using ContainerPtr = std::shared_ptr<Container>;
using CreaturePtr = std::shared_ptr<Creature>;
using MonsterPtr = std::shared_ptr<Monster>;
using NpcPtr = std::shared_ptr<Npc>;
using PlayerPtr = std::shared_ptr<Player>;
using LocalPlayerPtr = std::shared_ptr<LocalPlayer>;
using EffectPtr = std::shared_ptr<Effect>;
using MissilePtr = std::shared_ptr<Missile>;
using AnimatedTextPtr = std::shared_ptr<AnimatedText>;
using StaticTextPtr = std::shared_ptr<StaticText>;
using AnimatorPtr = std::shared_ptr<Animator>;
using ThingTypePtr = std::shared_ptr<ThingType>;
using ItemTypePtr = std::shared_ptr<ItemType>;
using HousePtr = std::shared_ptr<House>;
using TownPtr = std::shared_ptr<Town>;
using CreatureTypePtr = std::shared_ptr<CreatureType>;
using SpawnPtr = std::shared_ptr<Spawn>;

using ThingList = std::vector<ThingPtr>;
using ThingTypeList = std::vector<ThingTypePtr>;
using ItemTypeList = std::vector<ItemTypePtr>;
using HouseList = std::list<HousePtr>;
using TownList = std::list<TownPtr>;
using ItemList = std::list<ItemPtr>;
using TileList = std::list<TilePtr>;
using ItemVector = std::vector<ItemPtr>;
using TileMap = std::unordered_map<Position, TilePtr, PositionHasher>;
using CreatureMap = std::unordered_map<Position, CreatureTypePtr, PositionHasher>;
using SpawnMap = std::unordered_map<Position, SpawnPtr, PositionHasher>;

// net
class ProtocolLogin;
class ProtocolGame;

using ProtocolGamePtr = std::shared_ptr<ProtocolGame>;
using ProtocolLoginPtr = std::shared_ptr<ProtocolLogin>;

// ui
class UIItem;
class UICreature;
class UIGraph;
class UIMap;
class UIMinimap;
class UIProgressRect;
class UIMapAnchorLayout;
class UIPositionAnchor;
class UISprite;

using UIItemPtr = std::shared_ptr<UIItem>;
using UICreaturePtr = std::shared_ptr<UICreature>;
using UIGraphPtr = std::shared_ptr<UIGraph>;
using UISpritePtr = std::shared_ptr<UISprite>;
using UIMapPtr = std::shared_ptr<UIMap>;
using UIMinimapPtr = std::shared_ptr<UIMinimap>;
using UIProgressRectPtr = std::shared_ptr<UIProgressRect>;
using UIMapAnchorLayoutPtr = std::shared_ptr<UIMapAnchorLayout>;
using UIPositionAnchorPtr = std::shared_ptr<UIPositionAnchor>;

// custom
class HealthBar;

using HealthBarPtr = std::shared_ptr<HealthBar>;

#endif
