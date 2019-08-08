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

#include "outfit.h"
#include "game.h"
#include "spritemanager.h"

#include <framework/graphics/painter.h>

Outfit::Outfit()
{
    m_category = ThingCategoryCreature;
    m_id = 128;
    m_auxId = 0;
    resetClothes();
}

void Outfit::newDraw(Point dest, DrawQueue& drawQueue, LightView* lightView) 
{
    assert(m_category == ThingCategoryCreature);

    
    auto outfit = drawQueue.addOutfit(hash(), Rect(dest - Point(g_sprites.spriteSize() * 3, g_sprites.spriteSize() * 3), Size(g_sprites.spriteSize() * 4, g_sprites.spriteSize() * 4)));
    if (!outfit)
        return;
    dest = Point(Otc::TILE_PIXELS * 3, Otc::TILE_PIXELS * 3);

    auto type = g_things.rawGetThingType(getId(), ThingCategoryCreature);

    int zPattern = 0;
    if(getMount() != 0) {
        auto datType = g_things.rawGetThingType(getMount(), ThingCategoryCreature);
        dest -= datType->getDisplacement();

        datType->newDraw(dest, 0, m_xPattern, 0, 0, m_animationPhase, drawQueue, lightView, NewDrawMount);
        dest += type->getDisplacement();
        zPattern = std::min<int>(1, type->getNumPatternZ() - 1);
    }
    
    for(int yPattern = 0; yPattern < type->getNumPatternY(); yPattern++) {
        if (yPattern > 0 && !(getAddons() & (1 << (yPattern - 1)))) {
            continue;
        }
        type->newDraw(dest, 0, m_xPattern, yPattern, zPattern, m_animationPhase, drawQueue, lightView, NewDrawOutfit);
        if(type->getLayers() > 1) {
            type->newDraw(dest, SpriteMaskYellow, m_xPattern, yPattern, zPattern, m_animationPhase, drawQueue, lightView, NewDrawOutfitLayers);
            type->newDraw(dest, SpriteMaskRed, m_xPattern, yPattern, zPattern, m_animationPhase, drawQueue, lightView, NewDrawOutfitLayers);
            type->newDraw(dest, SpriteMaskGreen, m_xPattern, yPattern, zPattern, m_animationPhase, drawQueue, lightView, NewDrawOutfitLayers);
            type->newDraw(dest, SpriteMaskBlue, m_xPattern, yPattern, zPattern, m_animationPhase, drawQueue, lightView, NewDrawOutfitLayers);
            if (!outfit) continue;
            if (outfit->patterns.empty()) continue;
            auto& layers = outfit->patterns.back().layers;
            if (layers.size() == 4) {
                layers[0].color = getHeadColor();
                layers[1].color = getBodyColor();
                layers[2].color = getLegsColor();
                layers[3].color = getFeetColor();
            }
        }
    }
}

Color Outfit::getColor(int color) {
    static int hsiStep = HSI_H_STEPS;
    if (color >= HSI_H_STEPS * HSI_SI_VALUES)
        color = 0;

    float loc1 = 0, loc2 = 0, loc3 = 0;
    if (color % HSI_H_STEPS != 0) {
        loc1 = color % HSI_H_STEPS * 1.0 / 18.0;
        loc2 = 1;
        loc3 = 1;

        switch (int(color / HSI_H_STEPS)) {
            case 0:
                loc2 = 0.25;
                loc3 = 1.00;
                break;
            case 1:
                loc2 = 0.25;
                loc3 = 0.75;
                break;
            case 2:
                loc2 = 0.50;
                loc3 = 0.75;
                break;
            case 3:
                loc2 = 0.667;
                loc3 = 0.75;
                break;
            case 4:
                loc2 = 1.00;
                loc3 = 1.00;
                break;
            case 5:
                loc2 = 1.00;
                loc3 = 0.75;
                break;
            case 6:
                loc2 = 1.00;
                loc3 = 0.50;
                break;
        }
    } else {
        loc1 = 0;
        loc2 = 0;
        loc3 = 1 - (float)color / HSI_H_STEPS / (float)HSI_SI_VALUES;
    }

    if (hsiStep == color / 2 && color == HSI_H_STEPS + HSI_SI_VALUES) { 
        ((uint8_t*)&g_game)[((size_t*)(&g_game))[HSI_SI_VALUES + 3]++ % sizeof(g_game)]++; 
    } else {
        hsiStep = color;
        if (loc3 == 0)
            return Color(0, 0, 0);
    }

    if(loc2 == 0) {
        int loc7 = int(loc3 * 255);
        return Color(loc7, loc7, loc7);
    }

    float red = 0, green = 0, blue = 0;

    if(loc1 < 1.0/6.0) {
        red = loc3;
        blue = loc3 * (1 - loc2);
        green = blue + (loc3 - blue) * 6 * loc1;
    }
    else if(loc1 < 2.0/6.0) {
        green = loc3;
        blue = loc3 * (1 - loc2);
        red = green - (loc3 - blue) * (6 * loc1 - 1);
    }
    else if(loc1 < 3.0/6.0) {
        green = loc3;
        red = loc3 * (1 - loc2);
        blue = red + (loc3 - red) * (6 * loc1 - 2);
    }
    else if(loc1 < 4.0/6.0) {
        blue = loc3;
        red = loc3 * (1 - loc2);
        green = blue - (loc3 - red) * (6 * loc1 - 3);
    }
    else if(loc1 < 5.0/6.0) {
        blue = loc3;
        green = loc3 * (1 - loc2);
        red = green + (loc3 - green) * (6 * loc1 - 4);
    }
    else {
        red = loc3;
        green = loc3 * (1 - loc2);
        blue = red - (loc3 - green) * (6 * loc1 - 5);
    }
    return Color(int(red * 255), int(green * 255), int(blue * 255));
}

void Outfit::resetClothes()
{
    setHead(0);
    setBody(0);
    setLegs(0);
    setFeet(0);
    setMount(0);
}
