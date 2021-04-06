// hidden code 

#include "localplayer.h"
#include "map.h"
#include "game.h"
#include "tile.h"
#include <framework/core/eventdispatcher.h>
#include <framework/graphics/graphics.h>
#include <framework/util/extras.h>

bool LocalPlayer::canWalk(Otc::Direction direction, bool ignoreLock)
{
    // cannot walk while locked
    if ((m_walkLockExpiration != 0 && g_clock.millis() < m_walkLockExpiration) && !ignoreLock)
        return false;

    // paralyzed
    if (m_speed == 0)
        return false;

    // last walk is not done yet
    if (m_walking && (m_walkTimer.ticksElapsed() < getStepDuration()) && !isAutoWalking() && !isServerWalking())
        return false;

    auto tile = g_map.getTile(getPrewalkingPosition(true));
    if (isPreWalking() && (!m_lastPrewalkDone || (tile && tile->isBlocking())))
        return false;

    // cannot walk while already walking
    if ((m_walking && !isAutoWalking() && !isServerWalking()) && (!isPreWalking() || !m_lastPrewalkDone))
        return false;

    // Without new walking limit only to 1 prewalk
    if (!m_preWalking.empty() && !g_game.getFeature(Otc::GameNewWalking))
        return false;

    // Limit pre walking steps
    if (m_preWalking.size() >= g_game.getMaxPreWalkingSteps()) { // max 3 extra steps
        if (m_walkTimer.ticksElapsed() >= getStepDuration() + 300)
            return true;
        return false;
    }

    if (!m_preWalking.empty()) { // disallow diagonal extented prewalking walking
        auto dir = m_position.getDirectionFromPosition(m_preWalking.back());
        if ((dir == Otc::NorthWest || dir == Otc::NorthEast || dir == Otc::SouthWest || dir == Otc::SouthEast)) {
            return false;
        }
        if (!g_map.getTile(getPrewalkingPosition())->isWalkable())
            return false;
    }

    return true;
}

void LocalPlayer::walk(const Position& oldPos, const Position& newPos)
{
    if (g_extras.debugWalking) {
        g_logger.info(stdext::format("[%i] LocalPlayer::walk", (int)g_clock.millis()));
    }

    m_lastAutoWalkRetries = 0;
    // a prewalk was going on
    if (isPreWalking()) {
        for (auto it = m_preWalking.begin(); it != m_preWalking.end(); ++it) {
            if (*it == newPos) {
                m_preWalking.erase(m_preWalking.begin(), ++it);
                if (!isPreWalking()) // reset pre walking
                    updateWalk();
                return;
            }
        }
        if (g_extras.debugWalking) {
            g_logger.info(stdext::format("[%i] LocalPlayer::walk invalid prewalk", (int)g_clock.millis()));
        }

        // invalid pre walk
        m_preWalking.clear();
        m_serverWalking = true;
        if (m_serverWalkEndEvent)
            m_serverWalkEndEvent->cancel();

        Creature::walk(oldPos, newPos);
    } else { // no prewalk was going on, this must be an server side automated walk
        if (g_extras.debugWalking) {
            g_logger.info(stdext::format("[%i] LocalPlayer::walk server walk", (int)g_clock.millis()));
        }

        m_serverWalking = true;
        if (m_serverWalkEndEvent)
            m_serverWalkEndEvent->cancel();
        m_lastAutoWalkRetries = 0;

        Creature::walk(oldPos, newPos);
    }
}

void LocalPlayer::preWalk(Otc::Direction direction)
{
    Position startPos = m_position;
    if (!m_preWalking.empty())
        startPos = m_preWalking.back();
    Position newPos = startPos.translatedToDirection(direction);

    if (m_serverWalkEndEvent)
        m_serverWalkEndEvent->cancel();

    m_lastPrewalkDone = false;
    m_preWalking.push_back(newPos);
    if (m_preWalking.size() > 1)
        g_map.requestVisibleTilesCacheUpdate();

    Creature::walk(startPos, newPos);
}

void LocalPlayer::cancelNewWalk(Otc::Direction dir)
{
    if (g_extras.debugWalking) {
        g_logger.info(stdext::format("[%i] cancelWalk", (int)g_clock.millis()));
    }

    bool clearedPrewalk = !m_preWalking.empty();

    m_preWalking.clear();
    g_map.requestVisibleTilesCacheUpdate();

    if (clearedPrewalk) {
        stopWalk();
    }

    m_idleTimer.restart();

    if (retryAutoWalk()) return;

    if (!g_game.isIgnoringServerDirection() || !g_game.getFeature(Otc::GameNewWalking)) {
        setDirection(dir);
    }
    callLuaField("onCancelWalk", dir);
}

bool LocalPlayer::predictiveCancelWalk(const Position& pos, uint32_t predictionId, Otc::Direction dir)
{
    if (g_extras.debugPredictiveWalking) {
        g_logger.info(stdext::format("[%i] predictiveCancelWalk: %i - %i", (int)g_clock.millis(), predictionId, (int)m_preWalking.size()));
    }

    m_walkMatrix.update(pos, predictionId); // for debugging, not used

    for (auto it = m_preWalking.begin(); it != m_preWalking.end(); ++it) {
        if (*it != pos) {
            continue;
        }
        if (g_extras.debugPredictiveWalking) {
            g_logger.info(stdext::format("[%i] predictiveCancelWalk: canceling walk", (int)g_clock.millis()));
        }

        if (it == m_preWalking.begin()) {
            cancelNewWalk(dir);
            return true;
        }
        while (it != m_preWalking.end()) {
            it = m_preWalking.erase(it);
        }

        m_walkTimer.restart();
        m_walkTimer.adjust(-(getStepDuration(true) + 50));
        updateWalk();
        g_map.requestVisibleTilesCacheUpdate();

        return true;
    }

    return false;
}

bool LocalPlayer::retryAutoWalk()
{
    if (m_autoWalkDestination.isValid()) {
        g_game.stop();
        auto self = asLocalPlayer();

        if (m_lastAutoWalkRetries <= 3) {
            if (m_autoWalkContinueEvent)
                m_autoWalkContinueEvent->cancel();
            m_autoWalkContinueEvent = g_dispatcher.scheduleEvent(std::bind(&LocalPlayer::autoWalk, asLocalPlayer(), m_autoWalkDestination, true), 200);
            self->m_lastAutoWalkRetries += 1;
            return true;
        } else {
            self->m_autoWalkDestination = Position();
        }
    }
    return false;
}


bool LocalPlayer::autoWalk(Position destination, bool retry)
{
    // reset state
    m_autoWalkDestination = Position();
    m_lastAutoWalkPosition = Position();
    if (m_autoWalkContinueEvent)
        m_autoWalkContinueEvent->cancel();
    m_autoWalkContinueEvent = nullptr;

    if (!retry)
        m_lastAutoWalkRetries = 0;

    if (destination == getPrewalkingPosition())
        return true;

    m_autoWalkDestination = destination;
    auto self(asLocalPlayer());
    g_map.findPathAsync(getPrewalkingPosition(), destination, [self](PathFindResult_ptr result) {
        if (self->m_autoWalkDestination != result->destination)
            return;
        if (g_extras.debugWalking) {
            g_logger.info(stdext::format("Async path search finished with complexity %i/50000", result->complexity));
        }

        if (result->status != Otc::PathFindResultOk) {
            if (self->m_lastAutoWalkRetries > 0 && self->m_lastAutoWalkRetries <= 3) { // try again in 300, 700, 1200 ms if canceled by server
                self->m_autoWalkContinueEvent = g_dispatcher.scheduleEvent(std::bind(&LocalPlayer::autoWalk, self, result->destination, true), 200 + self->m_lastAutoWalkRetries * 100);
                return;
            }
            self->m_autoWalkDestination = Position();
            self->callLuaField("onAutoWalkFail", result->status);
            return;
        }

        if (!g_game.getFeature(Otc::GameNewWalking) && result->path.size() > 127)
            result->path.resize(127);
        else if (result->path.size() > 4095)
            result->path.resize(4095);

        if (result->path.empty()) {
            self->m_autoWalkDestination = Position();
            self->callLuaField("onAutoWalkFail", result->status);
            return;
        }

        auto finalAutowalkPos = self->getPrewalkingPosition().translatedToDirections(result->path).back();
        if (self->m_autoWalkDestination != finalAutowalkPos) {
            self->m_lastAutoWalkPosition = finalAutowalkPos;
        }

        g_game.autoWalk(result->path, result->start);
    });

    if (!retry)
        lockWalk();
    return true;
}
