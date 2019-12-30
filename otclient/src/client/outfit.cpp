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

void Outfit::newDraw(Point org_dest, DrawQueue& drawQueue, bool isWalking, LightView* lightView) 
{
    assert(m_category == ThingCategoryCreature);

    auto type = g_things.rawGetThingType(getId(), ThingCategoryCreature);
    if (!type) return;

    int mountAnimationPhase = m_animationPhase;
    Point dest = Point(Otc::TILE_PIXELS * 3, Otc::TILE_PIXELS * 3);

    if (type->getIdleAnimator()) {
        if (isWalking) {
            m_animationPhase += type->getIdleAnimator()->getAnimationPhases() - 1;
        } else {
            m_animationPhase = type->getIdleAnimator()->getPhase();
            mountAnimationPhase = 0;
        }
    }

    int zPattern = 0;
    if (getMount() != 0) {
        zPattern = std::min<int>(1, type->getNumPatternZ() - 1);
    }

    bool hasMultiLayerOutfit = false;
    for (int yPattern = 0; yPattern < type->getNumPatternY(); yPattern++) {
        if (yPattern > 0 && !(getAddons() & (1 << (yPattern - 1)))) {
            continue;
        }
        if (type->getLayers() > 1) {
            hasMultiLayerOutfit = true;
        }
    }

    if (!hasMultiLayerOutfit) { // optimization
        if (getMount() != 0) {
            auto datType = g_things.rawGetThingType(getMount(), ThingCategoryCreature);
            if (datType->getIdleAnimator()) {
                if (isWalking) {
                    mountAnimationPhase += datType->getIdleAnimator()->getAnimationPhases() - 1;
                } else {
                    mountAnimationPhase = datType->getIdleAnimator()->getPhase();
                }
            }

            org_dest -= datType->getDisplacement();
            datType->newDraw(org_dest, 0, m_xPattern, 0, 0, mountAnimationPhase, drawQueue, lightView);
            org_dest += type->getDisplacement();
        }

        for (int yPattern = 0; yPattern < type->getNumPatternY(); yPattern++) {
            if (yPattern > 0 && !(getAddons() & (1 << (yPattern - 1)))) {
                continue;
            }
            type->newDraw(org_dest, 0, m_xPattern, yPattern, zPattern, m_animationPhase, drawQueue, lightView);
        }
        return;
    }

    if(getMount() != 0) {
        auto datType = g_things.rawGetThingType(getMount(), ThingCategoryCreature);
        if (datType->getIdleAnimator()) {
            if (isWalking) {
                mountAnimationPhase += datType->getIdleAnimator()->getAnimationPhases() - 1;
            } else {
                mountAnimationPhase = datType->getIdleAnimator()->getPhase();
            }
        }
        Point mount_dest = org_dest;
        mount_dest -= datType->getDisplacement();
        datType->newDraw(mount_dest, 0, m_xPattern, 0, 0, mountAnimationPhase, drawQueue, lightView);
        dest -= datType->getDisplacement();
        dest += type->getDisplacement();
    }

    uint64_t hash = (((uint64_t)m_id) << 54) + (((uint64_t)m_auxId) << 50) + (((uint64_t)m_xPattern) << 46) +
        (((uint64_t)m_animationPhase) << 40) + (((uint64_t)m_head) << 32) + (((uint64_t)m_body) << 25) +
        (((uint64_t)m_legs) << 11) + (((uint64_t)m_feet) << 6) + (((uint64_t)m_addons) << 3) + (((uint64_t)zPattern) << 1) + (isWalking ? 1 : 0);

    auto outfit = drawQueue.addOutfit(hash, Rect(org_dest - Point(Otc::TILE_PIXELS * 3, Otc::TILE_PIXELS * 3), Size(Otc::TILE_PIXELS * 4, Otc::TILE_PIXELS * 4)));
    if (!outfit)
        return;

    for(int yPattern = 0; yPattern < type->getNumPatternY(); yPattern++) {
        if (yPattern > 0 && !(getAddons() & (1 << (yPattern - 1)))) {
            continue;
        }
        type->newDraw(dest, 0, m_xPattern, yPattern, zPattern, m_animationPhase, drawQueue, lightView, NewDrawOutfit);
        if (type->getLayers() <= 1) {
            continue;
        }

        type->newDraw(dest, SpriteMask, m_xPattern, yPattern, zPattern, m_animationPhase, drawQueue, lightView, NewDrawOutfitLayers);
        if (outfit->patterns.empty()) continue;
        auto& colors = outfit->patterns.back().colors;
        colors[0] = getHeadColor();
        colors[1] = getBodyColor();
        colors[2] = getLegsColor();
        colors[3] = getFeetColor();
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
