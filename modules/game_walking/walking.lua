smartWalkDirs = {}
smartWalkDir = nil
wsadWalking = false
nextWalkDir = nil
lastWalkDir = nil
lastFinishedStep = 0
autoWalkEvent = nil
firstStep = true
walkLock = 0
walkEvent = nil
lastWalk = 0
lastTurn = 0
lastTurnDirection = 0
lastStop = 0
lastManualWalk = 0
autoFinishNextServerWalk = 0
turnKeys = {}
local loaded = false

function init()
  connect(g_game, { onGameStart = onGameStart, onTeleport = onTeleport })
  
  connect(LocalPlayer, {
    onPositionChange = onPositionChange,
    onWalk = onWalk,
    onWalkFinish = onWalkFinish,
    onCancelWalk = onCancelWalk
  })

  modules.game_interface.getRootPanel().onFocusChange = stopSmartWalk

  Keybind.new("Movement", "Go " .. DirectionString[North],
    {
      [CHAT_MODE.ON] = "",
      [CHAT_MODE.OFF] = "W"
    },
    {
      [CHAT_MODE.ON] = "Numpad8",
      [CHAT_MODE.OFF] = ""
    }, true)
  Keybind.new("Movement", "Go " .. DirectionString[East],
    {
      [CHAT_MODE.ON] = "",
      [CHAT_MODE.OFF] = "D"
    },
    {
      [CHAT_MODE.ON] = "Numpad6",
      [CHAT_MODE.OFF] = ""
    }, true)
  Keybind.new("Movement", "Go " .. DirectionString[South],
    {
      [CHAT_MODE.ON] = "",
      [CHAT_MODE.OFF] = "S"
    },
    {
      [CHAT_MODE.ON] = "Numpad2",
      [CHAT_MODE.OFF] = ""
    }, true)
  Keybind.new("Movement", "Go " .. DirectionString[West],
    {
      [CHAT_MODE.ON] = "",
      [CHAT_MODE.OFF] = "A"
    },
    {
      [CHAT_MODE.ON] = "Numpad4",
      [CHAT_MODE.OFF] = ""
    }, true)

  Keybind.new("Movement", "Go " .. DirectionString[NorthEast],
    {
      [CHAT_MODE.ON] = "Numpad9",
      [CHAT_MODE.OFF] = "E"
    },
    {
      [CHAT_MODE.ON] = "",
      [CHAT_MODE.OFF] = ""
    }, true)
  Keybind.new("Movement", "Go " .. DirectionString[SouthEast],
    {
      [CHAT_MODE.ON] = "Numpad3",
      [CHAT_MODE.OFF] = "C"
    },
    {
      [CHAT_MODE.ON] = "",
      [CHAT_MODE.OFF] = ""
    }, true)
  Keybind.new("Movement", "Go " .. DirectionString[SouthWest],
    {
      [CHAT_MODE.ON] = "Numpad1",
      [CHAT_MODE.OFF] = "Z"
    },
    {
      [CHAT_MODE.ON] = "",
      [CHAT_MODE.OFF] = ""
    }, true)
  Keybind.new("Movement", "Go " .. DirectionString[NorthWest],
    {
      [CHAT_MODE.ON] = "Numpad7",
      [CHAT_MODE.OFF] = "Q"
    },
    {
      [CHAT_MODE.ON] = "",
      [CHAT_MODE.OFF] = ""
    }, true)
end

function terminate()
  disconnect(g_game, { onGameStart = onGameStart, onTeleport = onTeleport })
  
  disconnect(LocalPlayer, {
    onPositionChange = onPositionChange,
    onWalk = onWalk,
    onWalkFinish = onWalkFinish
  })
  removeEvent(autoWalkEvent)
  stopSmartWalk()
  unbindKeys()
  disableWSAD()
  
  for dir = North, NorthWest do
    Keybind.delete("Movement", "Go " .. DirectionString[dir])
  end
  
  loaded = false
end

function onGameStart()
  if not loaded then
    bindKeys()
    loaded = true
  end
end

function bindKeys()
  bindDefaultWalkKey("Up", North)
  bindDefaultWalkKey("Right", East)
  bindDefaultWalkKey("Down", South)
  bindDefaultWalkKey("Left", West)

  bindTurnKeys()

  for dir = North, NorthWest do
    bindWalkKey(dir)
  end
end

function unbindKeys()
  unbindDefaultWalkKey("Up")
  unbindDefaultWalkKey("Right")
  unbindDefaultWalkKey("Down")
  unbindDefaultWalkKey("Left")

  unbindTurnKeys()

  for dir = North, NorthWest do
    unbindWalkKey(dir)
  end
end

function enableWSAD()
  if wsadWalking then
    return
  end
  wsadWalking = true
  local player = g_game.getLocalPlayer()
  if player then
    player:lockWalk(100) -- 100 ms walk lock for all directions
  end
end

function disableWSAD()
  if not wsadWalking then
    return
  end

  wsadWalking = false
end

function bindDefaultWalkKey(key, dir)
  local gameRootPanel = modules.game_interface.getRootPanel()
  g_keyboard.bindKeyDown(
    key,
    function()
      changeWalkDir(dir)
    end,
    gameRootPanel,
    true
  )
  g_keyboard.bindKeyUp(
    key,
    function()
      changeWalkDir(dir, true)
    end,
    gameRootPanel,
    true
  )
  g_keyboard.bindKeyPress(
    key,
    function(c, k, ticks)
      smartWalk(dir, ticks)
    end,
    gameRootPanel
  )

  bindTurnKey("Ctrl+" .. key, dir)
end

function unbindDefaultWalkKey(key)
  local gameRootPanel = modules.game_interface.getRootPanel()
  g_keyboard.unbindKeyDown(key, gameRootPanel)
  g_keyboard.unbindKeyUp(key, gameRootPanel)
  g_keyboard.unbindKeyPress(key, gameRootPanel)

  unbindTurnKey("Ctrl+" .. key)
end

function bindWalkKey(dir)
  local gameRootPanel = modules.game_interface.getRootPanel()
  Keybind.bind("Movement", "Go " .. DirectionString[dir], {
    {
      type = KEY_DOWN,
      callback = function() changeWalkDir(dir) end,
      alone = true
    },
    {
      type = KEY_UP,
      callback = function() changeWalkDir(dir, true) end,
      alone = true
    },
    {
      type = KEY_PRESS,
      callback = function(c, k, ticks) smartWalk(dir, ticks) end
    }
  }, gameRootPanel)
end

function unbindWalkKey(dir)
  Keybind.unbind("Movement", "Go " .. DirectionString[dir])
end

function bindTurnKey(key, dir)
  turnKeys[key] = dir
  local gameRootPanel = modules.game_interface.getRootPanel()
  g_keyboard.bindKeyDown(key, function() turn(dir, false) end, gameRootPanel)
  g_keyboard.bindKeyPress(key, function() turn(dir, true) end, gameRootPanel)
  g_keyboard.bindKeyUp(key, function()
    local player = g_game.getLocalPlayer()
    if player then player:lockWalk(200) end
  end, gameRootPanel)
end

function unbindTurnKey(key)
  turnKeys[key] = nil
  local gameRootPanel = modules.game_interface.getRootPanel()
  g_keyboard.unbindKeyDown(key, gameRootPanel)
  g_keyboard.unbindKeyPress(key, gameRootPanel)
  g_keyboard.unbindKeyUp(key, gameRootPanel)
end

function bindTurnKeys()
  for dir = North, West do
    local keys = Keybind.getKeybindKeys("Movement", "Go " .. DirectionString[dir])
    if keys.primary and keys.primary:len() > 0 then
      bindTurnKey("Ctrl+" .. keys.primary, dir)
    end
    if keys.secondary and keys.secondary:len() > 0 then
      bindTurnKey("Ctrl+" .. keys.secondary, dir)
    end
  end
end

function unbindTurnKeys()
  for dir = North, West do
    local keys = Keybind.getKeybindKeys("Movement", "Go " .. DirectionString[dir])
    if keys.primary and keys.primary:len() > 0 then
      unbindTurnKey("Ctrl+" .. keys.primary)
    end
    if keys.secondary and keys.secondary:len() > 0 then
      unbindTurnKey("Ctrl+" .. keys.secondary)
    end
  end
end

function stopSmartWalk()
  smartWalkDirs = {}
  smartWalkDir = nil
end

function changeWalkDir(dir, pop)
  while table.removevalue(smartWalkDirs, dir) do end
  if pop then
    if #smartWalkDirs == 0 then
      stopSmartWalk()
      return
    end
  else
    table.insert(smartWalkDirs, 1, dir)
  end

  smartWalkDir = smartWalkDirs[1]
  if modules.client_options.getOption('smartWalk') and #smartWalkDirs > 1 then
    for _,d in pairs(smartWalkDirs) do
      if (smartWalkDir == North and d == West) or (smartWalkDir == West and d == North) then
        smartWalkDir = NorthWest
        break
      elseif (smartWalkDir == North and d == East) or (smartWalkDir == East and d == North) then
        smartWalkDir = NorthEast
        break
      elseif (smartWalkDir == South and d == West) or (smartWalkDir == West and d == South) then
        smartWalkDir = SouthWest
        break
      elseif (smartWalkDir == South and d == East) or (smartWalkDir == East and d == South) then
        smartWalkDir = SouthEast
        break
      end
    end
  end
end

function smartWalk(dir, ticks)
  walkEvent = scheduleEvent(function() 
    if g_keyboard.getModifiers() == KeyboardNoModifier then
      local direction = smartWalkDir or dir
      walk(direction, ticks)
      return true
    end
    return false
  end, 20)
end

function canChangeFloorDown(pos)
  pos.z = pos.z + 1
  toTile = g_map.getTile(pos)
  return toTile and toTile:hasElevation(3)
end

function canChangeFloorUp(pos)
  pos.z = pos.z - 1
  toTile = g_map.getTile(pos)
  return toTile and toTile:isWalkable()
end

function onPositionChange(player, newPos, oldPos)
end

function onWalk(player, newPos, oldPos)
  if autoFinishNextServerWalk + 200 > g_clock.millis() then
    player:finishServerWalking()
  end
end

function onTeleport(player, newPos, oldPos)
  if not newPos or not oldPos then
    return
  end
  -- floor change is also teleport
  if math.abs(newPos.x - oldPos.x) >= 3 or math.abs(newPos.y - oldPos.y) >= 3 or math.abs(newPos.z - oldPos.z) >= 2 then  
    -- far teleport, lock walk for 100ms
    walkLock = g_clock.millis() + g_settings.getNumber('walkTeleportDelay')
  else
    walkLock = g_clock.millis() + g_settings.getNumber('walkStairsDelay')
  end
  nextWalkDir = nil -- cancel autowalk
end

function onWalkFinish(player)
  lastFinishedStep = g_clock.millis()
  if nextWalkDir ~= nil then
    removeEvent(autoWalkEvent)
    autoWalkEvent = addEvent(function() if nextWalkDir ~= nil then walk(nextWalkDir, 0) end end, false)
  end
end

function onCancelWalk(player)
  player:lockWalk(50)
end

function walk(dir, ticks) 
  lastManualWalk = g_clock.millis()
  local player = g_game.getLocalPlayer()
  if not player or g_game.isDead() or player:isDead() then
    return
  end

  if player:isWalkLocked() then
    nextWalkDir = nil
    return
  end

  if g_game.isFollowing() then
    g_game.cancelFollow()
  end

  if player:isAutoWalking() then
    if lastStop + 100 < g_clock.millis() then
      lastStop = g_clock.millis()
      player:stopAutoWalk()
      g_game.stop()
    end
  end
     
  local dash = false
  local ignoredCanWalk = false
  if not g_game.getFeature(GameNewWalking) then
    dash = g_settings.getBoolean("dash", false)
  end

  local ticksToNextWalk = player:getStepTicksLeft()
  if not player:canWalk(dir) then -- canWalk return false when previous walk is not finished or not confirmed by server
    if dash then 
      ignoredCanWalk = true
    else
      if ticksToNextWalk < 500 and (lastWalkDir ~= dir or ticks == 0) then
        nextWalkDir = dir
      end
      if ticksToNextWalk < 30 and lastFinishedStep + 400 > g_clock.millis() and nextWalkDir == nil then -- clicked walk 20 ms too early, try to execute again as soon possible to keep smooth walking
        nextWalkDir = dir
      end
      return
    end
  end
  
  --if nextWalkDir ~= nil and lastFinishedStep + 200 < g_clock.millis() then
  --  print("Cancel " .. nextWalkDir)
  --  nextWalkDir = nil
  --end
  if nextWalkDir ~= nil and nextWalkDir ~= lastWalkDir then 
    dir = nextWalkDir
  end

  local toPos = player:getPrewalkingPosition(true)
  if dir == North then
    toPos.y = toPos.y - 1
  elseif dir == East then
    toPos.x = toPos.x + 1
  elseif dir == South then
    toPos.y = toPos.y + 1
  elseif dir == West then
    toPos.x = toPos.x - 1
  elseif dir == NorthEast then
    toPos.x = toPos.x + 1
    toPos.y = toPos.y - 1
  elseif dir == SouthEast then
    toPos.x = toPos.x + 1
    toPos.y = toPos.y + 1
  elseif dir == SouthWest then
    toPos.x = toPos.x - 1
    toPos.y = toPos.y + 1
  elseif dir == NorthWest then
    toPos.x = toPos.x - 1
    toPos.y = toPos.y - 1
  end
  local toTile = g_map.getTile(toPos)

  if walkLock >= g_clock.millis() and lastWalkDir == dir then
    nextWalkDir = nil
    return
  end

  if firstStep and lastWalkDir == dir and lastWalk + g_settings.getNumber('walkFirstStepDelay') > g_clock.millis() then
    firstStep = false
    walkLock = lastWalk + g_settings.getNumber('walkFirstStepDelay')
    return
  end
  
  if dash and lastWalkDir == dir and lastWalk + 50 > g_clock.millis() then
    return
  end  
  
  firstStep = (not player:isWalking() and lastFinishedStep + 100 < g_clock.millis() and walkLock + 100 < g_clock.millis())
  if player:isServerWalking() and not dash then
    walkLock = walkLock + math.max(g_settings.getNumber('walkFirstStepDelay'), 100)
  end
  
  nextWalkDir = nil
  removeEvent(autoWalkEvent)
  autoWalkEvent = nil
  local preWalked = false
  if toTile and toTile:isWalkable() then
    if not player:isServerWalking() and not ignoredCanWalk then
      player:preWalk(dir)
      preWalked = true
    end
  else
    local playerTile = player:getTile()
    if (playerTile and playerTile:hasElevation(3) and canChangeFloorUp(toPos)) or canChangeFloorDown(toPos) or (toTile and toTile:isEmpty() and not toTile:isBlocking()) then
      player:lockWalk(100)
    elseif player:isServerWalking() then
      g_game.stop()
      return
    elseif not toTile then
      player:lockWalk(100) -- bug fix for missing stairs down on map
    else
      if g_app.isMobile() and dir <= Directions.West then 
        turn(dir, ticks > 0)
      end
      return -- not walkable tile
    end
  end

  if player:isServerWalking() and not dash then
    g_game.stop()
    player:finishServerWalking()
    autoFinishNextServerWalk = g_clock.millis() + 200
  end
  g_game.walk(dir, preWalked)  
  
  if not firstStep and lastWalkDir ~= dir then
    walkLock = g_clock.millis() + g_settings.getNumber('walkTurnDelay')    
  end
  
  lastWalkDir = dir
  lastWalk = g_clock.millis()
  return true
end

function turn(dir, repeated)
  local player = g_game.getLocalPlayer()
  if player:isWalking() and player:getWalkDirection() == dir and not player:isServerWalking() then
    return
  end
  
  removeEvent(walkEvent)
  
  if not repeated or (lastTurn + 100 < g_clock.millis()) then
    g_game.turn(dir)
    changeWalkDir(dir)
    lastTurn = g_clock.millis()
    if not repeated then
      lastTurn = g_clock.millis() + 50
    end
    lastTurnDirection = dir
    nextWalkDir = nil
    player:lockWalk(g_settings.getNumber('walkCtrlTurnDelay'))
  end
end

function checkTurn()
  for keys, direction in pairs(turnKeys) do
    if g_keyboard.areKeysPressed(keys) then
      turn(direction, false)
    end
  end
end
