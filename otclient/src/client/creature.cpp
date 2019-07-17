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

#include "creature.h"
#include "thingtypemanager.h"
#include "localplayer.h"
#include "map.h"
#include "tile.h"
#include "item.h"
#include "game.h"
#include "effect.h"
#include "luavaluecasts.h"
#include "lightview.h"

#include <framework/graphics/graphics.h>
#include <framework/core/eventdispatcher.h>
#include <framework/core/clock.h>
#include <framework/core/graphicalapplication.h>

#include <framework/graphics/paintershaderprogram.h>
#include <framework/graphics/ogl/shadersources.h>
#include <framework/graphics/texturemanager.h>
#include <framework/graphics/framebuffermanager.h>
#include "spritemanager.h"

#include <framework/util/stats.h>
#include <framework/util/extras.h>

std::array<double, Otc::LastSpeedFormula> Creature::m_speedFormula = { -1,-1,-1 };

Creature::Creature() : Thing()
{
    m_id = 0;
    m_healthPercent = 100;
    m_speed = 200;
    m_direction = Otc::South;
    m_walkDirection = Otc::South;
    m_walkAnimationPhase = 0;
    m_walkedPixels = 0;
    m_skull = Otc::SkullNone;
    m_shield = Otc::ShieldNone;
    m_emblem = Otc::EmblemNone;
    m_type = Proto::CreatureTypeUnknown;
    m_icon = Otc::NpcIconNone;
    m_lastStepDirection = Otc::InvalidDirection;
    m_nameCache.setFont(g_fonts.getFont("verdana-11px-rounded"));
    m_nameCache.setAlign(Fw::AlignTopCenter);
    m_footStep = 0;
    //m_speedFormula.fill(-1);
    m_outfitColor = Color::white;
}

void Creature::draw(const Point& dest, float scaleFactor, bool animate, LightView *lightView, bool lightOnly)
{
    if(!canBeSeen())
        return;

    Point animationOffset = animate ? m_walkOffset : Point(0,0);

    if (!lightOnly) {
        if (m_showTimedSquare && animate) {
            g_painter->setColor(m_timedSquareColor);
            g_painter->drawBoundingRect(Rect(dest + (animationOffset - getDisplacement() + 2)*scaleFactor, Size(28, 28)*scaleFactor), std::max<int>((int)(2 * scaleFactor), 1));
            g_painter->setColor(Color::white);
        }

        if (m_showStaticSquare && animate) {
            g_painter->setColor(m_staticSquareColor);
            g_painter->drawBoundingRect(Rect(dest + (animationOffset - getDisplacement())*scaleFactor, Size(Otc::TILE_PIXELS, Otc::TILE_PIXELS)*scaleFactor), std::max<int>((int)(2 * scaleFactor), 1));
            g_painter->setColor(Color::white);
        }

        internalDrawOutfit(dest + animationOffset * scaleFactor, scaleFactor, animate, animate, m_walking ? m_walkDirection : m_direction);
        m_footStepDrawn = true;
    }
}

void Creature::newDraw(const Point& dest, DrawQueue& drawQueue, LightView* lightView) {
    if(!canBeSeen())
        return;

    if (m_showTimedSquare)
        drawQueue.addBoundingRect(Rect(dest + m_walkOffset - getDisplacement() + 2, Size(g_sprites.spriteSize() - 4, g_sprites.spriteSize() - 4)), m_timedSquareColor, 2);
    if (m_showStaticSquare)
        drawQueue.addBoundingRect(Rect(dest + m_walkOffset - getDisplacement(), Size(g_sprites.spriteSize(), g_sprites.spriteSize())), m_staticSquareColor, 2);

    newDrawOutfit(dest + m_walkOffset, drawQueue, lightView);
    m_footStepDrawn = true;

    if(lightView) {
        Light light = rawGetThingType()->getLight();
        if(m_light.intensity != light.intensity || m_light.color != light.color)
            light = m_light;

        // local player always have a minimum light in complete darkness
        if(isLocalPlayer() && (g_map.getLight().intensity < 64 || m_position.z > Otc::SEA_FLOOR)) {
            light.intensity = std::max<uint8>(light.intensity, 3);
            if(light.color == 0 || light.color > 215)
                light.color = 215;
        }

        if(light.intensity > 0)
            lightView->addLightSource(dest + (m_walkOffset + Point(8,8)), light, true);
    }
}

void Creature::internalDrawOutfit(Point dest, float scaleFactor, bool animateWalk, bool animateIdle, Otc::Direction direction, LightView *lightView, bool lightOnly)
{
    g_painter->setColor(m_outfitColor);

    // outfit is a real creature
    if(m_outfit.getCategory() == ThingCategoryCreature) {
        int animationPhase = animateWalk ? m_walkAnimationPhase : 0;

        if(isAnimateAlways() && animateIdle) {
            int ticksPerFrame = 1000 / getAnimationPhases();
            animationPhase = (g_clock.millis() % (ticksPerFrame * getAnimationPhases())) / ticksPerFrame;
        }

        // xPattern => creature direction
        int xPattern;
        if(direction == Otc::NorthEast || direction == Otc::SouthEast)
            xPattern = Otc::East;
        else if(direction == Otc::NorthWest || direction == Otc::SouthWest)
            xPattern = Otc::West;
        else
            xPattern = direction;

        int zPattern = 0;
        if(m_outfit.getMount() != 0) {
            auto datType = g_things.rawGetThingType(m_outfit.getMount(), ThingCategoryCreature);
            dest -= datType->getDisplacement() * scaleFactor;
            datType->draw(dest, scaleFactor, 0, xPattern, 0, 0, animationPhase, lightView, lightOnly);
            dest += getDisplacement() * scaleFactor;
            zPattern = std::min<int>(1, getNumPatternZ() - 1);
        }

        PointF jumpOffset = m_jumpOffset * scaleFactor;
        dest -= Point(stdext::round(jumpOffset.x), stdext::round(jumpOffset.y));

        // yPattern => creature addon
        for(int yPattern = 0; yPattern < getNumPatternY(); yPattern++) {

            // continue if we dont have this addon
            if(yPattern > 0 && !(m_outfit.getAddons() & (1 << (yPattern-1))))
                continue;

            auto datType = rawGetThingType();

            datType->draw(dest, scaleFactor, 0, xPattern, yPattern, zPattern, animationPhase, yPattern == 0 ? lightView : nullptr, lightOnly);

            if(getLayers() > 1 && !lightOnly) {
                Color oldColor = g_painter->getColor();
                Painter::CompositionMode oldComposition = g_painter->getCompositionMode();
                g_painter->setCompositionMode(Painter::CompositionMode_Multiply);
                g_painter->setColor(m_outfit.getHeadColor());
                datType->draw(dest, scaleFactor, SpriteMaskYellow, xPattern, yPattern, zPattern, animationPhase);
                g_painter->setColor(m_outfit.getBodyColor());
                datType->draw(dest, scaleFactor, SpriteMaskRed, xPattern, yPattern, zPattern, animationPhase);
                g_painter->setColor(m_outfit.getLegsColor());
                datType->draw(dest, scaleFactor, SpriteMaskGreen, xPattern, yPattern, zPattern, animationPhase);
                g_painter->setColor(m_outfit.getFeetColor());
                datType->draw(dest, scaleFactor, SpriteMaskBlue, xPattern, yPattern, zPattern, animationPhase);
                g_painter->setColor(oldColor);
                g_painter->setCompositionMode(oldComposition);
            }
        }
    // outfit is a creature imitating an item or the invisible effect
    } else  {
        ThingType *type = g_things.rawGetThingType(m_outfit.getAuxId(), m_outfit.getCategory());

        int animationPhase = 0;
        int animationPhases = type->getAnimationPhases();
        int animateTicks = Otc::ITEM_TICKS_PER_FRAME;

        // when creature is an effect we cant render the first and last animation phase,
        // instead we should loop in the phases between
        if(m_outfit.getCategory() == ThingCategoryEffect) {
            animationPhases = std::max<int>(1, animationPhases-2);
            animateTicks = Otc::INVISIBLE_TICKS_PER_FRAME;
        }

        if(animationPhases > 1) {
            if(animateIdle)
                animationPhase = (g_clock.millis() % (animateTicks * animationPhases)) / animateTicks;
            else
                animationPhase = animationPhases-1;
        }

        if(m_outfit.getCategory() == ThingCategoryEffect)
            animationPhase = std::min<int>(animationPhase+1, animationPhases);

        type->draw(dest - (getDisplacement() * scaleFactor), scaleFactor, 0, 0, 0, 0, animationPhase, lightView, lightOnly);
    }

    g_painter->resetColor();
}

void Creature::newDrawOutfit(const Point& dest, DrawQueue& drawQueue, LightView* lightView)
{
    if (m_outfit.getCategory() != ThingCategoryCreature) {
        ThingType *type = g_things.rawGetThingType(m_outfit.getAuxId(), m_outfit.getCategory());

        int animationPhase = 0;
        int animationPhases = type->getAnimationPhases();
        int animateTicks = Otc::ITEM_TICKS_PER_FRAME;

        if(m_outfit.getCategory() == ThingCategoryEffect) {
            animationPhases = std::max<int>(1, animationPhases-2);
            animateTicks = Otc::INVISIBLE_TICKS_PER_FRAME;
        }

        if(animationPhases > 1) {
            animationPhase = (g_clock.millis() % (animateTicks * animationPhases)) / animateTicks;
        }

        if(m_outfit.getCategory() == ThingCategoryEffect)
            animationPhase = std::min<int>(animationPhase+1, animationPhases);

        size_t hash = (m_outfit.getAuxId() << 16) + (m_outfit.getCategory() << 8) + animationPhase;
        auto outfit = drawQueue.addOutfit(hash, Rect(dest - getDisplacement() - Point(g_sprites.spriteSize(), g_sprites.spriteSize()), Size(g_sprites.spriteSize() * 2, g_sprites.spriteSize() * 2)));
        type->newDraw(Point(g_sprites.spriteSize(), g_sprites.spriteSize()), 0, 0, 0, 0, animationPhase, drawQueue, lightView, NewDrawOutfit);
    } else {
        int animationPhase = m_walkAnimationPhase;

        if (isAnimateAlways()) {
            int ticksPerFrame = 1000 / getAnimationPhases();
            animationPhase = (g_clock.millis() % (ticksPerFrame * getAnimationPhases())) / ticksPerFrame;
        }

        // xPattern => creature direction
        Otc::Direction dir = m_walking ? m_walkDirection : m_direction;
        int xPattern;
        if (dir == Otc::NorthEast || dir == Otc::SouthEast)
            xPattern = Otc::East;
        else if (dir == Otc::NorthWest || dir == Otc::SouthWest)
            xPattern = Otc::West;
        else
            xPattern = dir;

        m_outfit.setAnimationPhase(animationPhase);
        m_outfit.setXPattern(xPattern);

        m_outfit.newDraw(dest, drawQueue, lightView);
    }

    if (drawQueue.isBlocked())
        return;

    if (m_marked)
        drawQueue.setLastItemAsMarked(updatedMarkedColor());
}

void Creature::drawOutfit(const Rect& destRect, bool resize)
{
    int exactSize;
    if(m_outfit.getCategory() == ThingCategoryCreature)
        exactSize = getExactSize();
    else
        exactSize = g_things.rawGetThingType(m_outfit.getAuxId(), m_outfit.getCategory())->getExactSize();

    int frameSize;
    if(!resize)
        frameSize = std::max<int>(exactSize * 0.75f, 2 * Otc::TILE_PIXELS * 0.75f);
    else if(!(frameSize = exactSize))
        return;

    if(g_graphics.canUseFBO()) {
        const FrameBufferPtr& outfitBuffer = g_framebuffers.getTemporaryFrameBuffer();
        outfitBuffer->resize(Size(frameSize, frameSize));
        outfitBuffer->bind();
        g_painter->setAlphaWriting(true);
        g_painter->clear(Color::alpha);
        internalDrawOutfit(Point(frameSize - Otc::TILE_PIXELS, frameSize - Otc::TILE_PIXELS) + getDisplacement(), 1, false, true, Otc::South);
        outfitBuffer->release();
        outfitBuffer->draw(destRect, Rect(0,0,frameSize,frameSize));
    } else {
        float scaleFactor = destRect.width() / (float)frameSize;
        Point dest = destRect.bottomRight() - (Point(Otc::TILE_PIXELS,Otc::TILE_PIXELS) - getDisplacement()) * scaleFactor;
        internalDrawOutfit(dest, scaleFactor, false, true, Otc::South);
    }
}

void Creature::drawInformation(const Point& point, bool useGray, const Rect& parentRect, int drawFlags, DrawQueue& drawQueue)
{
    if(m_healthPercent < 1) // creature is dead
        return;

    Color fillColor = Color(96, 96, 96);

    if(!useGray)
        fillColor = m_informationColor;

    // calculate main rects
    Rect backgroundRect = Rect(point.x-(13.5), point.y, 27, 4);
    backgroundRect.bind(parentRect);

    //debug            
    if (g_extras.debugWalking) {
        int footDelay = (getStepDuration(true)) / 3;
        int footAnimPhases = getAnimationPhases() - 1;
        m_nameCache.setText(stdext::format("%i %i %i %i %i\n %i %i\n%i %i %i %i %i", (int)m_stepDuration, (int)getStepDuration(true), getStepDuration(false), (int)m_walkedPixels, (int)m_walkTimer.ticksElapsed(),
                                          (int)m_walkOffset.x, (int)m_walkOffset.y,
                                          (int)m_footTimer.ticksElapsed(), (int)footDelay, (int)footAnimPhases, (int)m_walkAnimationPhase, (int)stdext::millis()));
    }

    Size nameSize = m_nameCache.getTextSize();
    Rect textRect = Rect(point.x - nameSize.width() / 2.0, point.y-12, nameSize);
    textRect.bind(parentRect);

    // distance them
    uint32 offset = 12;
    if(isLocalPlayer()) {
        offset *= 2;
    }

    if(textRect.top() == parentRect.top())
        backgroundRect.moveTop(textRect.top() + offset);
    if(backgroundRect.bottom() == parentRect.bottom())
        textRect.moveTop(backgroundRect.top() - offset);

    // health rect is based on background rect, so no worries
    Rect healthRect = backgroundRect.expanded(-1);
    healthRect.setWidth((m_healthPercent / 100.0) * 25);

    // draw
    if(g_game.getFeature(Otc::GameBlueNpcNameColor) && isNpc() && m_healthPercent == 100 && !useGray)
        fillColor = Color(0x66, 0xcc, 0xff);

    if(drawFlags & Otc::DrawBars && (!isNpc() || !g_game.getFeature(Otc::GameHideNpcNames))) {
        drawQueue.add(backgroundRect, nullptr, Rect(0, 0, 1, 1), Color::black);
        drawQueue.add(healthRect, nullptr, Rect(0, 0, 1, 1), fillColor);

        if(drawFlags & Otc::DrawManaBar && isLocalPlayer()) {
            LocalPlayerPtr player = g_game.getLocalPlayer();
            if(player) {
                backgroundRect.moveTop(backgroundRect.bottom());
                drawQueue.add(backgroundRect, nullptr, Rect(0, 0, 1, 1), Color::black);

                Rect manaRect = backgroundRect.expanded(-1);
                double maxMana = player->getMaxMana();
                if(maxMana == 0) {
                    manaRect.setWidth(25);
                } else {
                    manaRect.setWidth(player->getMana() / (maxMana * 1.0) * 25);
                }

                drawQueue.add(manaRect, nullptr, Rect(0, 0, 1, 1), Color::blue);
            }
        }
    }

    if(drawFlags & Otc::DrawNames) {
        g_painter->setColor(fillColor);
        m_nameCache.draw(textRect);
    }

    if(m_skull != Otc::SkullNone && m_skullTexture) {
        Rect skullRect = Rect(backgroundRect.x() + 13.5 + 12, backgroundRect.y() + 5, m_skullTexture->getSize());
        drawQueue.add(skullRect, m_skullTexture, Rect(0, 0, m_skullTexture->getSize()), Color::white);
    }
    if(m_shield != Otc::ShieldNone && m_shieldTexture && m_showShieldTexture) {
        Rect shieldRect = Rect(backgroundRect.x() + 13.5, backgroundRect.y() + 5, m_shieldTexture->getSize());
        drawQueue.add(shieldRect, m_shieldTexture, Rect(0, 0, m_shieldTexture->getSize()), Color::white);
    }
    if(m_emblem != Otc::EmblemNone && m_emblemTexture) {
        Rect emblemRect = Rect(backgroundRect.x() + 13.5 + 12, backgroundRect.y() + 16, m_emblemTexture->getSize());
        drawQueue.add(emblemRect, m_emblemTexture, Rect(0, 0, m_emblemTexture->getSize()), Color::white);
    }
    if(m_type != Proto::CreatureTypeUnknown && m_typeTexture) {
        Rect typeRect = Rect(backgroundRect.x() + 13.5 + 12 + 12, backgroundRect.y() + 16, m_typeTexture->getSize());
        drawQueue.add(typeRect, m_typeTexture, Rect(0, 0, m_typeTexture->getSize()), Color::white);
    }
    if(m_icon != Otc::NpcIconNone && m_iconTexture) {
        Rect iconRect = Rect(backgroundRect.x() + 13.5 + 12, backgroundRect.y() + 5, m_iconTexture->getSize());
        drawQueue.add(iconRect, m_iconTexture, Rect(0, 0, m_iconTexture->getSize()), Color::white);
    }
}

void Creature::turn(Otc::Direction direction)
{
    setDirection(direction);
}

void Creature::walk(const Position& oldPos, const Position& newPos)
{
    if(oldPos == newPos)
        return;

    // get walk direction
    m_lastStepDirection = oldPos.getDirectionFromPosition(newPos);
    m_lastStepFromPosition = oldPos;
    m_lastStepToPosition = newPos;

    // set current walking direction
    setDirection(m_lastStepDirection);
    m_walkDirection = m_direction;

    // starts counting walk
    m_walking = true;
    m_walkTimer.restart();
    m_walkedPixels = 0;

    if(m_walkFinishAnimEvent) {
        m_walkFinishAnimEvent->cancel();
        m_walkFinishAnimEvent = nullptr;
    }

    // no direction need to be changed when the walk ends
    m_stepDuration = 0;
    if (m_nextStepSpeed > 0) {
        m_stepDuration = m_nextStepSpeed;
        m_nextStepSpeed = 0;
    }

    // starts updating walk
    nextWalkUpdate();
}

void Creature::stopWalk(bool)
{
    if(!m_walking)
        return;

    // stops the walk right away
    terminateWalk();
}

void Creature::jump(int height, int duration)
{
    if(!m_jumpOffset.isNull())
        return;

    m_jumpTimer.restart();
    m_jumpHeight = height;
    m_jumpDuration = duration;

    updateJump();
}

void Creature::updateJump()
{
    int t = m_jumpTimer.ticksElapsed();
    double a = -4 * m_jumpHeight / (m_jumpDuration * m_jumpDuration);
    double b = +4 * m_jumpHeight / (m_jumpDuration);

    double height = a*t*t + b*t;
    int roundHeight = stdext::round(height);
    int halfJumpDuration = m_jumpDuration / 2;

    // schedules next update
    if(m_jumpTimer.ticksElapsed() < m_jumpDuration) {
        m_jumpOffset = PointF(height, height);

        int diff = 0;
        if(m_jumpTimer.ticksElapsed() < halfJumpDuration)
            diff = 1;
        else if(m_jumpTimer.ticksElapsed() > halfJumpDuration)
            diff = -1;

        int nextT, i = 1;
        do {
            nextT = stdext::round((-b + std::sqrt(std::max<double>(b*b + 4*a*(roundHeight+diff*i), 0.0)) * diff) / (2*a));
            ++i;

            if(nextT < halfJumpDuration)
                diff = 1;
            else if(nextT > halfJumpDuration)
                diff = -1;
        } while(nextT - m_jumpTimer.ticksElapsed() == 0 && i < 3);

        auto self = static_self_cast<Creature>();
        g_dispatcher.scheduleEvent([self] {
            self->updateJump();
        }, nextT - m_jumpTimer.ticksElapsed());
    }
    else
        m_jumpOffset = PointF(0, 0);
}

void Creature::onPositionChange(const Position& newPos, const Position& oldPos)
{
    callLuaField("onPositionChange", newPos, oldPos);
}

void Creature::onAppear()
{
    // cancel any disappear event
    if(m_disappearEvent) {
        m_disappearEvent->cancel();
        m_disappearEvent = nullptr;
    }

    // creature appeared the first time or wasn't seen for a long time
    if(m_removed) {
        stopWalk();
        m_removed = false;
        callLuaField("onAppear");
    // walk
    } else if(m_oldPosition != m_position && m_oldPosition.isInRange(m_position,1,1) && m_allowAppearWalk) {
        m_allowAppearWalk = false;
        walk(m_oldPosition, m_position);
        callLuaField("onWalk", m_oldPosition, m_position);
    // teleport
    } else if(m_oldPosition != m_position) {
        stopWalk(m_oldPosition.distance(m_position) >= 3);
        callLuaField("onDisappear");
        callLuaField("onAppear");
    } // else turn

    m_nextStepSpeed = 0;
}

void Creature::onDisappear()
{
    if(m_disappearEvent)
        m_disappearEvent->cancel();

    m_oldPosition = m_position;

    // a pair onDisappear and onAppear events are fired even when creatures walks or turns,
    // so we must filter
    auto self = static_self_cast<Creature>();
    m_disappearEvent = g_dispatcher.addEvent([self] {
        self->m_removed = true;
        self->stopWalk();

        self->callLuaField("onDisappear");

        // invalidate this creature position
        if(!self->isLocalPlayer())
            self->setPosition(Position());
        self->m_oldPosition = Position();
        self->m_disappearEvent = nullptr;
    });
}

void Creature::onDeath()
{
    callLuaField("onDeath");
}

void Creature::updateWalkAnimation(int totalPixelsWalked)
{
    // update outfit animation
    if(m_outfit.getCategory() != ThingCategoryCreature)
        return;

    int footAnimPhases = getAnimationPhases() - 1;
    // TODO, should be /2 for <= 810
    int footDelay = getStepDuration(true) / 3; //(m_stepDuration ? m_stepDuration : getStepDuration(true)) / 3;
    // Since mount is a different outfit we need to get the mount animation phases
    if(m_outfit.getMount() != 0) {
        ThingType *type = g_things.rawGetThingType(m_outfit.getMount(), m_outfit.getCategory());
        footAnimPhases = std::min<int>(footAnimPhases, type->getAnimationPhases() - 1);
    }
    if(footAnimPhases == 0)
        m_walkAnimationPhase = 0;
    else if(m_footStepDrawn && m_footTimer.ticksElapsed() >= footDelay && totalPixelsWalked < 32) {
        m_footStep++;
        m_walkAnimationPhase = 1 + (m_footStep % footAnimPhases);
        m_footStepDrawn = false;
        m_footTimer.restart();
    } else if(m_walkAnimationPhase == 0 && totalPixelsWalked < 32) {
        m_walkAnimationPhase = 1 + (m_footStep % footAnimPhases);
    }

    if(totalPixelsWalked == 32 && !m_walkFinishAnimEvent) {
        auto self = static_self_cast<Creature>();
        m_walkFinishAnimEvent = g_dispatcher.scheduleEvent([self] {
//            if(!self->m_walking || self->m_walkTimer.ticksElapsed() >= (self->m_stepDuration ? self->m_stepDuration : self->getStepDuration(true)))
            if(!self->m_walking || self->m_walkTimer.ticksElapsed() >= (self->getStepDuration(true)))
                self->m_walkAnimationPhase = 0;
            self->m_walkFinishAnimEvent = nullptr;
        }, std::min<int>(std::max<int>(footDelay, 50), 100));
    }

}

void Creature::updateWalkOffset(int totalPixelsWalked)
{
    m_walkOffset = Point(0,0);
    if(m_walkDirection == Otc::North || m_walkDirection == Otc::NorthEast || m_walkDirection == Otc::NorthWest)
        m_walkOffset.y = 32 - totalPixelsWalked;
    else if(m_walkDirection == Otc::South || m_walkDirection == Otc::SouthEast || m_walkDirection == Otc::SouthWest)
        m_walkOffset.y = totalPixelsWalked - 32;

    if(m_walkDirection == Otc::East || m_walkDirection == Otc::NorthEast || m_walkDirection == Otc::SouthEast)
        m_walkOffset.x = totalPixelsWalked - 32;
    else if(m_walkDirection == Otc::West || m_walkDirection == Otc::NorthWest || m_walkDirection == Otc::SouthWest)
        m_walkOffset.x = 32 - totalPixelsWalked;
}

void Creature::updateWalkingTile()
{
    // determine new walking tile
    TilePtr newWalkingTile;
    Rect virtualCreatureRect(g_sprites.spriteSize() + (m_walkOffset.x - getDisplacementX()),
        g_sprites.spriteSize() + (m_walkOffset.y - getDisplacementY()),
        g_sprites.spriteSize(), g_sprites.spriteSize());
    for(int xi = -1; xi <= 1 && !newWalkingTile; ++xi) {
        for(int yi = -1; yi <= 1 && !newWalkingTile; ++yi) {
            Rect virtualTileRect((xi+1)* g_sprites.spriteSize(), (yi+1)* g_sprites.spriteSize(), g_sprites.spriteSize(), g_sprites.spriteSize());

            // only render creatures where bottom right is inside tile rect
            if(virtualTileRect.contains(virtualCreatureRect.bottomRight())) {
                newWalkingTile = g_map.getOrCreateTile(getNewPreWalkingPosition().translated(xi, yi, 0));
            }
        }
    }

    if(newWalkingTile != m_walkingTile) {
        if(m_walkingTile)
            m_walkingTile->removeWalkingCreature(static_self_cast<Creature>());
        if(newWalkingTile) {
            newWalkingTile->addWalkingCreature(static_self_cast<Creature>());

            // recache visible tiles in map views
            if(newWalkingTile->isEmpty())
                g_map.notificateTileUpdate(newWalkingTile->getPosition());
        }
        m_walkingTile = newWalkingTile;
    }
}

void Creature::nextWalkUpdate()
{
    // remove any previous scheduled walk updates
    if(m_walkUpdateEvent)
        m_walkUpdateEvent->cancel();

    // do the update
    updateWalk();

    // schedules next update
    if(m_walking) {
        auto self = static_self_cast<Creature>();
        m_walkUpdateEvent = g_dispatcher.scheduleEvent([self] {
            self->m_walkUpdateEvent = nullptr;
            self->nextWalkUpdate();
        }, getStepDuration() / 32);
    }
}

void Creature::updateWalk()
{
    float walkTicksPerPixel = getStepDuration(true) / 32;
    int totalPixelsWalked = std::min<int>(m_walkTimer.ticksElapsed() / walkTicksPerPixel, 32.0f);

    // needed for paralyze effect
    m_walkedPixels = std::max<int>(m_walkedPixels, totalPixelsWalked);

    // update walk animation and offsets
    updateWalkAnimation(totalPixelsWalked);
    updateWalkOffset(m_walkedPixels);
    updateWalkingTile();

    // terminate walk
    if(m_walking && m_walkTimer.ticksElapsed() >= getStepDuration())
        terminateWalk();
}

void Creature::terminateWalk()
{
    // remove any scheduled walk update
    if(m_walkUpdateEvent) {
        m_walkUpdateEvent->cancel();
        m_walkUpdateEvent = nullptr;
    }

    if(m_walkingTile) {
        m_walkingTile->removeWalkingCreature(static_self_cast<Creature>());
        m_walkingTile = nullptr;
    }

    m_walking = false;
    m_walkedPixels = 0;

    // reset walk animation states
    m_walkOffset = Point(0,0);
    m_walkAnimationPhase = 0;
}

void Creature::setName(const std::string& name)
{
    m_nameCache.setText(name);
    m_name = name;
}

void Creature::setHealthPercent(uint8 healthPercent)
{
    if(healthPercent > 92)
        m_informationColor = Color(0x00, 0xBC, 0x00);
    else if(healthPercent > 60)
        m_informationColor = Color(0x50, 0xA1, 0x50);
    else if(healthPercent > 30)
        m_informationColor = Color(0xA1, 0xA1, 0x00);
    else if(healthPercent > 8)
        m_informationColor = Color(0xBF, 0x0A, 0x0A);
    else if(healthPercent > 3)
        m_informationColor = Color(0x91, 0x0F, 0x0F);
    else
        m_informationColor = Color(0x85, 0x0C, 0x0C);

    m_healthPercent = healthPercent;
    callLuaField("onHealthPercentChange", healthPercent);

    if(healthPercent <= 0)
        onDeath();
}

void Creature::setDirection(Otc::Direction direction)
{
    assert(direction != Otc::InvalidDirection);
    m_direction = direction;
}

void Creature::setOutfit(const Outfit& outfit)
{
    Outfit oldOutfit = m_outfit;
    if(outfit.getCategory() != ThingCategoryCreature) {
        if(!g_things.isValidDatId(outfit.getAuxId(), outfit.getCategory()))
            return;
        m_outfit.setAuxId(outfit.getAuxId());
        m_outfit.setCategory(outfit.getCategory());
    } else {
        if(outfit.getId() > 0 && !g_things.isValidDatId(outfit.getId(), ThingCategoryCreature))
            return;
        m_outfit = outfit;
    }
    m_walkAnimationPhase = 0; // might happen when player is walking and outfit is changed.

    callLuaField("onOutfitChange", m_outfit, oldOutfit);
}

void Creature::setOutfitColor(const Color& color, int duration)
{
    if(m_outfitColorUpdateEvent) {
        m_outfitColorUpdateEvent->cancel();
        m_outfitColorUpdateEvent = nullptr;
    }

    if(duration > 0) {
        Color delta = (color - m_outfitColor) / (float)duration;
        m_outfitColorTimer.restart();
        updateOutfitColor(m_outfitColor, color, delta, duration);
    }
    else
        m_outfitColor = color;
}

void Creature::updateOutfitColor(Color color, Color finalColor, Color delta, int duration)
{
    if(m_outfitColorTimer.ticksElapsed() < duration) {
        m_outfitColor = color + delta * m_outfitColorTimer.ticksElapsed();

        auto self = static_self_cast<Creature>();
        m_outfitColorUpdateEvent = g_dispatcher.scheduleEvent([=] {
            self->updateOutfitColor(color, finalColor, delta, duration);
        }, 100);
    }
    else {
        m_outfitColor = finalColor;
    }
}

void Creature::setSpeed(uint16 speed)
{
    uint16 oldSpeed = m_speed;
    m_speed = speed;

    // speed can change while walking (utani hur, paralyze, etc..)
    if(m_walking)
        nextWalkUpdate();

    callLuaField("onSpeedChange", m_speed, oldSpeed);
}

void Creature::setBaseSpeed(double baseSpeed)
{
    if(m_baseSpeed != baseSpeed) {
        double oldBaseSpeed = m_baseSpeed;
        m_baseSpeed = baseSpeed;

        callLuaField("onBaseSpeedChange", baseSpeed, oldBaseSpeed);
    }
}

void Creature::setSkull(uint8 skull)
{
    m_skull = skull;
    callLuaField("onSkullChange", m_skull);
}

void Creature::setShield(uint8 shield)
{
    m_shield = shield;
    callLuaField("onShieldChange", m_shield);
}

void Creature::setEmblem(uint8 emblem)
{
    m_emblem = emblem;
    callLuaField("onEmblemChange", m_emblem);
}

void Creature::setType(uint8 type)
{
    m_type = type;
    callLuaField("onTypeChange", m_type);
}

void Creature::setIcon(uint8 icon)
{
    m_icon = icon;
    callLuaField("onIconChange", m_icon);
}

void Creature::setSkullTexture(const std::string& filename)
{
    m_skullTexture = g_textures.getTexture(filename);
}

void Creature::setShieldTexture(const std::string& filename, bool blink)
{
    m_shieldTexture = g_textures.getTexture(filename);
    m_showShieldTexture = true;

    if(blink && !m_shieldBlink) {
        auto self = static_self_cast<Creature>();
        g_dispatcher.scheduleEvent([self]() {
            self->updateShield();
        }, SHIELD_BLINK_TICKS);
    }

    m_shieldBlink = blink;
}

void Creature::setEmblemTexture(const std::string& filename)
{
    m_emblemTexture = g_textures.getTexture(filename);
}

void Creature::setTypeTexture(const std::string& filename)
{
    m_typeTexture = g_textures.getTexture(filename);
}

void Creature::setIconTexture(const std::string& filename)
{
    m_iconTexture = g_textures.getTexture(filename);
}

void Creature::setSpeedFormula(double speedA, double speedB, double speedC)
{
    m_speedFormula[Otc::SpeedFormulaA] = speedA;
    m_speedFormula[Otc::SpeedFormulaB] = speedB;
    m_speedFormula[Otc::SpeedFormulaC] = speedC;
}

bool Creature::hasSpeedFormula()
{
    return m_speedFormula[Otc::SpeedFormulaA] != -1 && m_speedFormula[Otc::SpeedFormulaB] != -1
            && m_speedFormula[Otc::SpeedFormulaC] != -1;
}

void Creature::addTimedSquare(uint8 color)
{
    m_showTimedSquare = true;
    m_timedSquareColor = Color::from8bit(color);

    // schedule removal
    auto self = static_self_cast<Creature>();
    g_dispatcher.scheduleEvent([self]() {
        self->removeTimedSquare();
    }, VOLATILE_SQUARE_DURATION);
}

void Creature::updateShield()
{
    m_showShieldTexture = !m_showShieldTexture;

    if(m_shield != Otc::ShieldNone && m_shieldBlink) {
        auto self = static_self_cast<Creature>();
        g_dispatcher.scheduleEvent([self]() {
            self->updateShield();
        }, SHIELD_BLINK_TICKS);
    }
    else if(!m_shieldBlink)
        m_showShieldTexture = true;
}

Point Creature::getDrawOffset()
{
    Point drawOffset;
    if(m_walking) {
        if(m_walkingTile)
            drawOffset -= Point(1,1) * m_walkingTile->getDrawElevation();
        drawOffset += m_walkOffset;
    } else {
        const TilePtr& tile = getTile();
        if(tile)
            drawOffset -= Point(1,1) * tile->getDrawElevation();
    }
    return drawOffset;
}

int Creature::getStepDuration(bool ignoreDiagonal, Otc::Direction dir)
{
    int speed = m_speed;
    if(speed < 1)
        return 0;

    if(g_game.getFeature(Otc::GameNewSpeedLaw))
        speed *= 2;

    int groundSpeed = 0;
    Position tilePos;

    if(dir == Otc::InvalidDirection)
        tilePos = m_lastStepToPosition;
    else
        tilePos = getNewPreWalkingPosition(true).translatedToDirection(dir);

    if(!tilePos.isValid())
        tilePos = getNewPreWalkingPosition(true);

    const TilePtr& tile = g_map.getTile(tilePos);
    if(tile) {
        groundSpeed = tile->getGroundSpeed();
        if(groundSpeed == 0)
            groundSpeed = 150;
    }

    int interval = 1000;
    if(groundSpeed > 0 && speed > 0)
        interval = 1000 * groundSpeed;

    if(g_game.getFeature(Otc::GameNewSpeedLaw) && hasSpeedFormula()) {
        int formulatedSpeed = 1;
        if(speed > -m_speedFormula[Otc::SpeedFormulaB]) {
            formulatedSpeed = std::max<int>(1, (int)floor((m_speedFormula[Otc::SpeedFormulaA] * log((speed / 2)
                 + m_speedFormula[Otc::SpeedFormulaB]) + m_speedFormula[Otc::SpeedFormulaC]) + 0.5));
        }
        interval = std::floor(interval / (double)formulatedSpeed);
    }
    else
        interval /= speed;

    if (g_game.getClientVersion() >= 900) {
        interval = std::ceil((float)interval / (float)g_game.getServerBeat()) * g_game.getServerBeat();
    }

    float factor = 3;
    if(g_game.getClientVersion() <= 810)
        factor = 2;

    interval = std::max<int>(interval, g_game.getServerBeat());

    if(!ignoreDiagonal && (m_lastStepDirection == Otc::NorthWest || m_lastStepDirection == Otc::NorthEast ||
       m_lastStepDirection == Otc::SouthWest || m_lastStepDirection == Otc::SouthEast))
        interval *= factor;

    if (!isServerWalking()) {
        interval += g_game.getFeature(Otc::GameSlowerManualWalking) ? 25 : 5;
    }
    return interval;
}

Point Creature::getDisplacement()
{
    if(m_outfit.getCategory() == ThingCategoryEffect)
        return Point(8, 8);
    else if(m_outfit.getCategory() == ThingCategoryItem)
        return Point(0, 0);
    return Thing::getDisplacement();
}

int Creature::getDisplacementX()
{
    if(m_outfit.getCategory() == ThingCategoryEffect)
        return 8;
    else if(m_outfit.getCategory() == ThingCategoryItem)
        return 0;

    if(m_outfit.getMount() != 0) {
        auto datType = g_things.rawGetThingType(m_outfit.getMount(), ThingCategoryCreature);
        return datType->getDisplacementX();
    }

    return Thing::getDisplacementX();
}

int Creature::getDisplacementY()
{
    if(m_outfit.getCategory() == ThingCategoryEffect)
        return 8;
    else if(m_outfit.getCategory() == ThingCategoryItem)
        return 0;

    if(m_outfit.getMount() != 0) {
        auto datType = g_things.rawGetThingType(m_outfit.getMount(), ThingCategoryCreature);
        return datType->getDisplacementY();
    }

    return Thing::getDisplacementY();
}

int Creature::getExactSize(int layer, int xPattern, int yPattern, int zPattern, int animationPhase)
{
    int exactSize = 0;

    animationPhase = 0;
    xPattern = Otc::South;

    zPattern = 0;
    if(m_outfit.getMount() != 0)
        zPattern = 1;

    for(yPattern = 0; yPattern < getNumPatternY(); yPattern++) {
        if(yPattern > 0 && !(m_outfit.getAddons() & (1 << (yPattern-1))))
            continue;

        for(layer = 0; layer < getLayers(); ++layer)
            exactSize = std::max<int>(exactSize, Thing::getExactSize(layer, xPattern, yPattern, zPattern, animationPhase));
    }

    return exactSize;
}

const ThingTypePtr& Creature::getThingType()
{
    return g_things.getThingType(m_outfit.getId(), ThingCategoryCreature);
}

ThingType* Creature::rawGetThingType()
{
    return g_things.rawGetThingType(m_outfit.getId(), ThingCategoryCreature);
}
