WALK_STEPS_RETRY = 10

gameRootPanel = nil
gameMapPanel = nil
gameRightPanel = nil
gameLeftPanel = nil
gameThirdPanel = nil
gameForthPanel = nil
gameBottomPanel = nil
healthBar = nil
manaBar = nil
showTopMenuButton = nil
logoutButton = nil
mouseGrabberWidget = nil
countWindow = nil
logoutWindow = nil
exitWindow = nil
bottomSplitter = nil
limitedZoom = false
currentViewMode = 0
smartWalkDirs = {}
smartWalkDir = nil
hookedMenuOptions = {}
lastDirTime = g_clock.millis()
healthCircle = nil
healthCircleFront = nil
manaCircle = nil
manaCircleFront = nil
markedThing = nil

function init()
  g_ui.importStyle('styles/countwindow')

  connect(g_game, {
    onGameStart = onGameStart,
    onGameEnd = onGameEnd,
    onLoginAdvice = onLoginAdvice,
  }, true)

  -- Call load AFTER game window has been created and 
  -- resized to a stable state, otherwise the saved 
  -- settings can get overridden by false onGeometryChange
  -- events
  connect(g_app, {
    onRun = load,
    onExit = save
  })
  
  gameRootPanel = g_ui.displayUI('gameinterface')
  gameRootPanel:hide()
  gameRootPanel:lower()
  gameRootPanel.onGeometryChange = updateStretchShrink
  gameRootPanel.onFocusChange = stopSmartWalk

  mouseGrabberWidget = gameRootPanel:getChildById('mouseGrabber')
  mouseGrabberWidget.onMouseRelease = onMouseGrabberRelease

  bottomSplitter = gameRootPanel:getChildById('bottomSplitter')
  gameMapPanel = gameRootPanel:getChildById('gameMapPanel')
  gameRightPanel = gameRootPanel:getChildById('gameRightPanel')
  gameLeftPanel = gameRootPanel:getChildById('gameLeftPanel')
  gameBottomPanel = gameRootPanel:getChildById('gameBottomPanel')
  gameThirdPanel = gameRootPanel:getChildById('gameThirdPanel')
  gameForthPanel = gameRootPanel:getChildById('gameForthPanel')
  connect(gameLeftPanel, { onVisibilityChange = onLeftPanelVisibilityChange })
  connect(gameThirdPanel, { onVisibilityChange = onThirdPanelVisibilityChange })
  connect(gameForthPanel, { onVisibilityChange = onForthPanelVisibilityChange })

  healthBar = gameMapPanel:getChildById('healthBar')
  manaBar = gameMapPanel:getChildById('manaBar')

  healthCircle = gameRootPanel:getChildById('healthCircle')
  healthCircleFront = gameRootPanel:getChildById('healthCircleFront')
  manaCircle = gameRootPanel:getChildById('manaCircle')
  manaCircleFront = gameRootPanel:getChildById('manaCircleFront')

  logoutButton = modules.client_topmenu.addLeftButton('logoutButton', tr('Exit'),
    '/images/topbuttons/logout', tryLogout, true)

  showTopMenuButton = gameMapPanel:getChildById('showTopMenuButton')
  showTopMenuButton.onClick = function()
    modules.client_topmenu.toggle()
  end
  
  setupViewMode(0)

  bindKeys()
  
  connect(gameMapPanel, { onGeometryChange = updateSize })

  if g_game.isOnline() then
    show()
  end
end

function bindKeys()
  gameRootPanel:setAutoRepeatDelay(200)

  bindWalkKey('Up', North)
  bindWalkKey('Right', East)
  bindWalkKey('Down', South)
  bindWalkKey('Left', West)
  bindWalkKey('Numpad8', North)
  bindWalkKey('Numpad9', NorthEast)
  bindWalkKey('Numpad6', East)
  bindWalkKey('Numpad3', SouthEast)
  bindWalkKey('Numpad2', South)
  bindWalkKey('Numpad1', SouthWest)
  bindWalkKey('Numpad4', West)
  bindWalkKey('Numpad7', NorthWest)

  bindTurnKey('Ctrl+Up', North)
  bindTurnKey('Ctrl+Right', East)
  bindTurnKey('Ctrl+Down', South)
  bindTurnKey('Ctrl+Left', West)
  bindTurnKey('Ctrl+Numpad8', North)
  bindTurnKey('Ctrl+Numpad6', East)
  bindTurnKey('Ctrl+Numpad2', South)
  bindTurnKey('Ctrl+Numpad4', West)

  g_keyboard.bindKeyPress('Escape', function() g_game.cancelAttackAndFollow() end, gameRootPanel)
  g_keyboard.bindKeyPress('Ctrl+=', function() gameMapPanel:zoomIn() end, gameRootPanel)
  g_keyboard.bindKeyPress('Ctrl+-', function() gameMapPanel:zoomOut() end, gameRootPanel)
  g_keyboard.bindKeyDown('Ctrl+Q', function() tryLogout(false) end, gameRootPanel)
  g_keyboard.bindKeyDown('Ctrl+L', function() tryLogout(false) end, gameRootPanel)
  g_keyboard.bindKeyDown('Ctrl+W', function() g_map.cleanTexts() modules.game_textmessage.clearMessages() end, gameRootPanel)
  g_keyboard.bindKeyDown('Ctrl+.', nextViewMode, gameRootPanel)
end

function bindWalkKey(key, dir)
  g_keyboard.bindKeyDown(key, function() changeWalkDir(dir) end, gameRootPanel, true)
  g_keyboard.bindKeyUp(key, function() changeWalkDir(dir, true) end, gameRootPanel, true)
  g_keyboard.bindKeyPress(key, function() smartWalk(dir) end, gameRootPanel)
end

function unbindWalkKey(key)
  g_keyboard.unbindKeyDown(key, gameRootPanel)
  g_keyboard.unbindKeyUp(key, gameRootPanel)
  g_keyboard.unbindKeyPress(key, gameRootPanel)
end

function bindTurnKey(key, dir)
  local function callback(widget, code, repeatTicks)
    if g_clock.millis() - lastDirTime >= modules.client_options.getOption('turnDelay') then
        g_game.turn(dir)
        changeWalkDir(dir)

        lastDirTime = g_clock.millis()
    end
  end

  g_keyboard.bindKeyPress(key, callback, gameRootPanel)
end

function terminate()
  hide()

  hookedMenuOptions = {}
  markThing = nil
  
  stopSmartWalk()

  disconnect(g_game, {
    onGameStart = onGameStart,
    onGameEnd = onGameEnd,
    onLoginAdvice = onLoginAdvice
  })

  disconnect(gameLeftPanel, { onVisibilityChange = onLeftPanelVisibilityChange })
  disconnect(gameThirdPanel, { onVisibilityChange = onThirdPanelVisibilityChange })
  disconnect(gameForthPanel, { onVisibilityChange = onForthPanelVisibilityChange })
  disconnect(gameMapPanel, { onGeometryChange = updateSize })

  logoutButton:destroy()
  gameRootPanel:destroy()
end

function onGameStart()
  show()

  -- open tibia has delay in auto walking
  if not g_game.isOfficialTibia() then
    g_game.enableFeature(GameForceFirstAutoWalkStep)
  else
    g_game.disableFeature(GameForceFirstAutoWalkStep)
  end
end

function onGameEnd()
  setupViewMode(0)
  hide()
end

function show()
  connect(g_app, { onClose = tryExit })
  modules.client_background.hide()
  gameRootPanel:show()
  gameRootPanel:focus()
  gameMapPanel:followCreature(g_game.getLocalPlayer())
    
  updateStretchShrink()
  logoutButton:setTooltip(tr('Logout'))
  
  local classicView = g_settings.getBoolean('classicView')
  if classicView then
    setupViewMode(0)
  else
    setupViewMode(1)  
  end  

  addEvent(function()
    if not limitedZoom or g_game.isGM() then
      gameMapPanel:setMaxZoomOut(513)
      gameMapPanel:setLimitVisibleRange(false)
    else
      gameMapPanel:setMaxZoomOut(15)
      gameMapPanel:setLimitVisibleRange(true)
    end
  end)
end

function hide()
  disconnect(g_app, { onClose = tryExit })
  logoutButton:setTooltip(tr('Exit'))

  if logoutWindow then
    logoutWindow:destroy()
    logoutWindow = nil
  end
  if exitWindow then
    exitWindow:destroy()
    exitWindow = nil
  end
  if countWindow then
    countWindow:destroy()
    countWindow = nil
  end
  gameRootPanel:hide()
  modules.client_background.show()
end

function save()
  local settings = {}
  settings.splitterMarginBottom = bottomSplitter:getMarginBottom()
  g_settings.setNode('game_interface', settings)
end

function load()
  local settings = g_settings.getNode('game_interface')
  if settings then
    if settings.splitterMarginBottom then
      bottomSplitter:setMarginBottom(settings.splitterMarginBottom)
    end
  end
end

function onLoginAdvice(message)
  displayInfoBox(tr("For Your Information"), message)
end

function forceExit()
  g_game.cancelLogin()
  scheduleEvent(exit, 10)
  return true
end

function tryExit()
  if exitWindow then
    return true
  end

  local exitFunc = function() g_game.safeLogout() forceExit() end
  local logoutFunc = function() g_game.safeLogout() exitWindow:destroy() exitWindow = nil end
  local cancelFunc = function() exitWindow:destroy() exitWindow = nil end

  exitWindow = displayGeneralBox(tr('Exit'), tr("If you shut down the program, your character might stay in the game.\nClick on 'Logout' to ensure that you character leaves the game properly.\nClick on 'Exit' if you want to exit the program without logging out your character."),
  { { text=tr('Force Exit'), callback=exitFunc },
    { text=tr('Logout'), callback=logoutFunc },
    { text=tr('Cancel'), callback=cancelFunc },
    anchor=AnchorHorizontalCenter }, logoutFunc, cancelFunc)

  return true
end

function tryLogout(prompt)
  if type(prompt) ~= "boolean" then
    prompt = true
  end
  if not g_game.isOnline() then
    exit()
    return
  end

  if logoutWindow then
    return
  end

  local msg, yesCallback
  if not g_game.isConnectionOk() then
    msg = 'Your connection is failing, if you logout now your character will be still online, do you want to force logout?'

    yesCallback = function()
      g_game.forceLogout()
      if logoutWindow then
        logoutWindow:destroy()
        logoutWindow=nil
      end
    end
  else
    msg = 'Are you sure you want to logout?'

    yesCallback = function()
      g_game.safeLogout()
      if logoutWindow then
        logoutWindow:destroy()
        logoutWindow=nil
      end
    end
  end

  local noCallback = function()
    logoutWindow:destroy()
    logoutWindow=nil
  end

  if prompt then
    logoutWindow = displayGeneralBox(tr('Logout'), tr(msg), {
      { text=tr('Yes'), callback=yesCallback },
      { text=tr('No'), callback=noCallback },
      anchor=AnchorHorizontalCenter}, yesCallback, noCallback)
  else
     yesCallback()
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

function smartWalk(dir)
  local localPlayer = g_game.getLocalPlayer()
  if localPlayer:isPreWalking() and not g_settings.getBoolean('extentedPreWalking') then
    return false
  end
  if g_keyboard.getModifiers() == KeyboardNoModifier then
    local dire = smartWalkDir or dir
    g_game.walk(dire)
    return true
  end
  return false
end

function updateStretchShrink()
  if modules.client_options.getOption('dontStretchShrink') and not alternativeView then
    gameMapPanel:setVisibleDimension({ width = 15, height = 11 })

    -- Set gameMapPanel size to height = 11 * 32 + 2
    bottomSplitter:setMarginBottom(bottomSplitter:getMarginBottom() + (gameMapPanel:getHeight() - 32 * 11) - 10)
  end
end

function onMouseGrabberRelease(self, mousePosition, mouseButton)
  if selectedThing == nil then return false end
  if mouseButton == MouseLeftButton then
    local clickedWidget = gameRootPanel:recursiveGetChildByPos(mousePosition, false)
    if clickedWidget then
      if selectedType == 'use' then
        onUseWith(clickedWidget, mousePosition)
      elseif selectedType == 'trade' then
        onTradeWith(clickedWidget, mousePosition)
      end
    end
  end

  selectedThing = nil
  g_mouse.popCursor('target')
  self:ungrabMouse()
  return true
end

function onUseWith(clickedWidget, mousePosition)
  if clickedWidget:getClassName() == 'UIGameMap' then
    local tile = clickedWidget:getTile(mousePosition)
    if tile then
      if selectedThing:isFluidContainer() or selectedThing:isMultiUse() then
        g_game.useWith(selectedThing, tile:getTopMultiUseThing())
      else
        g_game.useWith(selectedThing, tile:getTopUseThing())
      end
    end
  elseif clickedWidget:getClassName() == 'UIItem' and not clickedWidget:isVirtual() then
    g_game.useWith(selectedThing, clickedWidget:getItem())
  elseif clickedWidget:getClassName() == 'UICreatureButton' then
    local creature = clickedWidget:getCreature()
    if creature then
      g_game.useWith(selectedThing, creature)
    end
  end
end

function onTradeWith(clickedWidget, mousePosition)
  if clickedWidget:getClassName() == 'UIGameMap' then
    local tile = clickedWidget:getTile(mousePosition)
    if tile then
      g_game.requestTrade(selectedThing, tile:getTopCreature())
    end
  elseif clickedWidget:getClassName() == 'UICreatureButton' then
    local creature = clickedWidget:getCreature()
    if creature then
      g_game.requestTrade(selectedThing, creature)
    end
  end
end

function startUseWith(thing)
  if not thing then return end
  if g_ui.isMouseGrabbed() then
    if selectedThing then
      selectedThing = thing
      selectedType = 'use'
    end
    return
  end
  selectedType = 'use'
  selectedThing = thing
  mouseGrabberWidget:grabMouse()
  g_mouse.pushCursor('target')
end

function startTradeWith(thing)
  if not thing then return end
  if g_ui.isMouseGrabbed() then
    if selectedThing then
      selectedThing = thing
      selectedType = 'trade'
    end
    return
  end
  selectedType = 'trade'
  selectedThing = thing
  mouseGrabberWidget:grabMouse()
  g_mouse.pushCursor('target')
end

function isMenuHookCategoryEmpty(category)
  if category then
    for _,opt in pairs(category) do
      if opt then return false end
    end
  end
  return true
end

function addMenuHook(category, name, callback, condition, shortcut)
  if not hookedMenuOptions[category] then
    hookedMenuOptions[category] = {}
  end
  hookedMenuOptions[category][name] = {
    callback = callback,
    condition = condition,
    shortcut = shortcut
  }
end

function removeMenuHook(category, name)
  if not name then
    hookedMenuOptions[category] = {}
  else
    hookedMenuOptions[category][name] = nil
  end
end

function createThingMenu(menuPosition, lookThing, useThing, creatureThing)
  if not g_game.isOnline() then return end

  local menu = g_ui.createWidget('PopupMenu')
  menu:setGameMenu(true)

  local classic = modules.client_options.getOption('classicControl')
  local shortcut = nil

  if not classic then shortcut = '(Shift)' else shortcut = nil end
  if lookThing then
    menu:addOption(tr('Look'), function() g_game.look(lookThing) end, shortcut)
  end

  if not classic then shortcut = '(Ctrl)' else shortcut = nil end
  if useThing then
    if useThing:isContainer() then
      if useThing:getParentContainer() then
        menu:addOption(tr('Open'), function() g_game.open(useThing, useThing:getParentContainer()) end, shortcut)
        menu:addOption(tr('Open in new window'), function() g_game.open(useThing) end)
      else
        menu:addOption(tr('Open'), function() g_game.open(useThing) end, shortcut)
      end
    else
      if useThing:isMultiUse() then
        menu:addOption(tr('Use with ...'), function() startUseWith(useThing) end, shortcut)
      else
        menu:addOption(tr('Use'), function() g_game.use(useThing) end, shortcut)
      end
    end

    if useThing:isRotateable() then
      menu:addOption(tr('Rotate'), function() g_game.rotate(useThing) end)
    end

    if g_game.getFeature(GameBrowseField) and useThing:getPosition().x ~= 0xffff then
      menu:addOption(tr('Browse Field'), function() g_game.browseField(useThing:getPosition()) end)
    end
  end

  if lookThing and not lookThing:isCreature() and not lookThing:isNotMoveable() and lookThing:isPickupable() then
    menu:addSeparator()
    menu:addOption(tr('Trade with ...'), function() startTradeWith(lookThing) end)
  end

  if lookThing then
    local parentContainer = lookThing:getParentContainer()
    if parentContainer and parentContainer:hasParent() then
      menu:addOption(tr('Move up'), function() g_game.moveToParentContainer(lookThing, lookThing:getCount()) end)
    end
  end

  if creatureThing then
    local localPlayer = g_game.getLocalPlayer()
    menu:addSeparator()

    if creatureThing:isLocalPlayer() then
      menu:addOption(tr('Set Outfit'), function() g_game.requestOutfit() end)

      if g_game.getFeature(GamePlayerMounts) then
        if not localPlayer:isMounted() then
          menu:addOption(tr('Mount'), function() localPlayer:mount() end)
        else
          menu:addOption(tr('Dismount'), function() localPlayer:dismount() end)
        end
      end

      if creatureThing:isPartyMember() then
        if creatureThing:isPartyLeader() then
          if creatureThing:isPartySharedExperienceActive() then
            menu:addOption(tr('Disable Shared Experience'), function() g_game.partyShareExperience(false) end)
          else
            menu:addOption(tr('Enable Shared Experience'), function() g_game.partyShareExperience(true) end)
          end
        end
        menu:addOption(tr('Leave Party'), function() g_game.partyLeave() end)
      end

    else
      local localPosition = localPlayer:getPosition()
      if not classic then shortcut = '(Alt)' else shortcut = nil end
      if creatureThing:getPosition().z == localPosition.z then
        if g_game.getAttackingCreature() ~= creatureThing then
          menu:addOption(tr('Attack'), function() g_game.attack(creatureThing) end, shortcut)
        else
          menu:addOption(tr('Stop Attack'), function() g_game.cancelAttack() end, shortcut)
        end

        if g_game.getFollowingCreature() ~= creatureThing then
          menu:addOption(tr('Follow'), function() g_game.follow(creatureThing) end)
        else
          menu:addOption(tr('Stop Follow'), function() g_game.cancelFollow() end)
        end
      end

      if creatureThing:isPlayer() then
        menu:addSeparator()
        local creatureName = creatureThing:getName()
        menu:addOption(tr('Message to %s', creatureName), function() g_game.openPrivateChannel(creatureName) end)
        if modules.game_console.getOwnPrivateTab() then
          menu:addOption(tr('Invite to private chat'), function() g_game.inviteToOwnChannel(creatureName) end)
          menu:addOption(tr('Exclude from private chat'), function() g_game.excludeFromOwnChannel(creatureName) end) -- [TODO] must be removed after message's popup labels been implemented
        end
        if not localPlayer:hasVip(creatureName) then
          menu:addOption(tr('Add to VIP list'), function() g_game.addVip(creatureName) end)
        end

        if modules.game_console.isIgnored(creatureName) then
          menu:addOption(tr('Unignore') .. ' ' .. creatureName, function() modules.game_console.removeIgnoredPlayer(creatureName) end)
        else
          menu:addOption(tr('Ignore') .. ' ' .. creatureName, function() modules.game_console.addIgnoredPlayer(creatureName) end)
        end

        local localPlayerShield = localPlayer:getShield()
        local creatureShield = creatureThing:getShield()

        if localPlayerShield == ShieldNone or localPlayerShield == ShieldWhiteBlue then
          if creatureShield == ShieldWhiteYellow then
            menu:addOption(tr('Join %s\'s Party', creatureThing:getName()), function() g_game.partyJoin(creatureThing:getId()) end)
          else
            menu:addOption(tr('Invite to Party'), function() g_game.partyInvite(creatureThing:getId()) end)
          end
        elseif localPlayerShield == ShieldWhiteYellow then
          if creatureShield == ShieldWhiteBlue then
            menu:addOption(tr('Revoke %s\'s Invitation', creatureThing:getName()), function() g_game.partyRevokeInvitation(creatureThing:getId()) end)
          end
        elseif localPlayerShield == ShieldYellow or localPlayerShield == ShieldYellowSharedExp or localPlayerShield == ShieldYellowNoSharedExpBlink or localPlayerShield == ShieldYellowNoSharedExp then
          if creatureShield == ShieldWhiteBlue then
            menu:addOption(tr('Revoke %s\'s Invitation', creatureThing:getName()), function() g_game.partyRevokeInvitation(creatureThing:getId()) end)
          elseif creatureShield == ShieldBlue or creatureShield == ShieldBlueSharedExp or creatureShield == ShieldBlueNoSharedExpBlink or creatureShield == ShieldBlueNoSharedExp then
            menu:addOption(tr('Pass Leadership to %s', creatureThing:getName()), function() g_game.partyPassLeadership(creatureThing:getId()) end)
          else
            menu:addOption(tr('Invite to Party'), function() g_game.partyInvite(creatureThing:getId()) end)
          end
        end
      end
    end

    if modules.game_ruleviolation.hasWindowAccess() and creatureThing:isPlayer() then
      menu:addSeparator()
      menu:addOption(tr('Rule Violation'), function() modules.game_ruleviolation.show(creatureThing:getName()) end)
    end

    menu:addSeparator()
    menu:addOption(tr('Copy Name'), function() g_window.setClipboardText(creatureThing:getName()) end)
  end

  -- hooked menu options
  for _,category in pairs(hookedMenuOptions) do
    if not isMenuHookCategoryEmpty(category) then
      menu:addSeparator()
      for name,opt in pairs(category) do
        if opt and opt.condition(menuPosition, lookThing, useThing, creatureThing) then
          menu:addOption(name, function() opt.callback(menuPosition, 
            lookThing, useThing, creatureThing) end, opt.shortcut)
        end
      end
    end
  end

  --if g_game.getFeature(GameBot) and useThing then
    menu:addSeparator()
    menu:addOption(tr("ID: " .. useThing:getId()))
    menu:addOption(tr("ID: " .. tostring(useThing:isTranslucent())))    
    menu:addOption(tr("ID: " .. tostring(useThing:isGround())))    
  --end

  menu:display(menuPosition)
end

function processMouseAction(menuPosition, mouseButton, autoWalkPos, lookThing, useThing, creatureThing, attackCreature, marking)
  local keyboardModifiers = g_keyboard.getModifiers()
  
  if not modules.client_options.getOption('classicControl') then
    if keyboardModifiers == KeyboardNoModifier and mouseButton == MouseRightButton then
      createThingMenu(menuPosition, lookThing, useThing, creatureThing)
      return true
    elseif lookThing and keyboardModifiers == KeyboardShiftModifier and (mouseButton == MouseLeftButton or mouseButton == MouseRightButton) then
      g_game.look(lookThing)
      return true
    elseif useThing and keyboardModifiers == KeyboardCtrlModifier and (mouseButton == MouseLeftButton or mouseButton == MouseRightButton) then
      if useThing:isContainer() then
        if useThing:getParentContainer() then
          g_game.open(useThing, useThing:getParentContainer())
        else
          g_game.open(useThing)
        end
        return true
      elseif useThing:isMultiUse() then
        startUseWith(useThing)
        return true
      else
        g_game.use(useThing)
        return true
      end
      return true
    elseif attackCreature and g_keyboard.isAltPressed() and (mouseButton == MouseLeftButton or mouseButton == MouseRightButton) then
      g_game.attack(attackCreature)
      return true
    elseif creatureThing and creatureThing:getPosition().z == autoWalkPos.z and g_keyboard.isAltPressed() and (mouseButton == MouseLeftButton or mouseButton == MouseRightButton) then
      g_game.attack(creatureThing)
      return true
    end

  -- classic control
  else
    if useThing and keyboardModifiers == KeyboardNoModifier and mouseButton == MouseRightButton and not g_mouse.isPressed(MouseLeftButton) then
      local player = g_game.getLocalPlayer()
      if attackCreature and attackCreature ~= player then
        g_game.attack(attackCreature)
        return true
      elseif creatureThing and creatureThing ~= player and creatureThing:getPosition().z == autoWalkPos.z then
        g_game.attack(creatureThing)
        return true
      elseif useThing:isContainer() then
        if useThing:getParentContainer() then
          g_game.open(useThing, useThing:getParentContainer())
          return true
        else
          g_game.open(useThing)
          return true
        end
      elseif useThing:isMultiUse() then
        startUseWith(useThing)
        return true
      else
        g_game.use(useThing)
        return true
      end
      return true
    elseif lookThing and keyboardModifiers == KeyboardShiftModifier and (mouseButton == MouseLeftButton or mouseButton == MouseRightButton) then
      g_game.look(lookThing)
      return true
    elseif lookThing and ((g_mouse.isPressed(MouseLeftButton) and mouseButton == MouseRightButton) or (g_mouse.isPressed(MouseRightButton) and mouseButton == MouseLeftButton)) then
      g_game.look(lookThing)
      return true
    elseif useThing and keyboardModifiers == KeyboardCtrlModifier and (mouseButton == MouseLeftButton or mouseButton == MouseRightButton) then
      createThingMenu(menuPosition, lookThing, useThing, creatureThing)
      return true
    elseif attackCreature and g_keyboard.isAltPressed() and (mouseButton == MouseLeftButton or mouseButton == MouseRightButton) then
      g_game.attack(attackCreature)
      return true
    elseif creatureThing and creatureThing:getPosition().z == autoWalkPos.z and g_keyboard.isAltPressed() and (mouseButton == MouseLeftButton or mouseButton == MouseRightButton) then
      g_game.attack(creatureThing)
      return true
    end
  end


  local player = g_game.getLocalPlayer()
  player:stopAutoWalk()

  if autoWalkPos and keyboardModifiers == KeyboardNoModifier and mouseButton == MouseLeftButton then
    player:autoWalk(autoWalkPos)
    return true
  end

  return false
end

function moveStackableItem(item, toPos)
  if countWindow then
    return
  end
  if g_keyboard.isCtrlPressed() then
    g_game.move(item, toPos, item:getCount())
    return
  elseif g_keyboard.isShiftPressed() then
    g_game.move(item, toPos, 1)
    return
  end
  local count = item:getCount()

  countWindow = g_ui.createWidget('CountWindow', rootWidget)
  local itembox = countWindow:getChildById('item')
  local scrollbar = countWindow:getChildById('countScrollBar')
  itembox:setItemId(item:getId())
  itembox:setItemCount(count)
  scrollbar:setMaximum(count)
  scrollbar:setMinimum(1)
  scrollbar:setValue(count)

  local spinbox = countWindow:getChildById('spinBox')
  spinbox:setMaximum(count)
  spinbox:setMinimum(0)
  spinbox:setValue(0)
  spinbox:hideButtons()
  spinbox:focus()
  spinbox.firstEdit = true

  local spinBoxValueChange = function(self, value)
    spinbox.firstEdit = false
    scrollbar:setValue(value)
  end
  spinbox.onValueChange = spinBoxValueChange

  local check = function()
    if spinbox.firstEdit then
      spinbox:setValue(spinbox:getMaximum())
      spinbox.firstEdit = false
    end
  end
  g_keyboard.bindKeyPress("Up", function() check() spinbox:up() end, spinbox)
  g_keyboard.bindKeyPress("Down", function() check() spinbox:down() end, spinbox)
  g_keyboard.bindKeyPress("Right", function() check() spinbox:up() end, spinbox)
  g_keyboard.bindKeyPress("Left", function() check() spinbox:down() end, spinbox)
  g_keyboard.bindKeyPress("PageUp", function() check() spinbox:setValue(spinbox:getValue()+10) end, spinbox)
  g_keyboard.bindKeyPress("PageDown", function() check() spinbox:setValue(spinbox:getValue()-10) end, spinbox)

  scrollbar.onValueChange = function(self, value)
    itembox:setItemCount(value)
    spinbox.onValueChange = nil
    spinbox:setValue(value)
    spinbox.onValueChange = spinBoxValueChange
  end

  local okButton = countWindow:getChildById('buttonOk')
  local moveFunc = function()
    g_game.move(item, toPos, itembox:getItemCount())
    okButton:getParent():destroy()
    countWindow = nil
  end
  local cancelButton = countWindow:getChildById('buttonCancel')
  local cancelFunc = function()
    cancelButton:getParent():destroy()
    countWindow = nil
  end

  countWindow.onEnter = moveFunc
  countWindow.onEscape = cancelFunc

  okButton.onClick = moveFunc
  cancelButton.onClick = cancelFunc
end

function getRootPanel()
  return gameRootPanel
end

function getMapPanel()
  return gameMapPanel
end

function getRightPanel()
  return gameRightPanel
end

function getLeftPanel()
  return gameLeftPanel
end

function getBottomPanel()
  return gameBottomPanel
end

function getThirdPanel()
  return gameThirdPanel
end

function getForthPanel()
  return gameForthPanel
end

function getShowTopMenuButton()
  return showTopMenuButton
end

function onLeftPanelVisibilityChange(leftPanel, visible)
  if not visible and g_game.isOnline() then
    local children = leftPanel:getChildren()
    for i=1,#children do
      children[i]:setParent(gameRightPanel)
    end
  end
end

function onThirdPanelVisibilityChange(thirdPanel, visible)
  if not visible and g_game.isOnline() then
    local children = thirdPanel:getChildren()
    for i=1,#children do
      children[i]:setParent(gameRightPanel)
    end
  end
end

function refreshViewMode()
  setupViewMode(currentViewMode)
end

function setupViewMode(mode)
  if not g_settings.getBoolean("allowFullView") then
    mode = 0
  end
  
  --[[
      self:addOption("Right")
      self:addOption("Left, Right")
      self:addOption("2x Right")
      self:addOption("Left, 2x Right")
      self:addOption("2x Left, 2x Right")
      self:addOption("Right + transparent left and right panels")
      self:addOption("Left, Right + transparent left and right panels")  
  ]]--
  
  local panels = modules.client_options.getOption('panels')

  if mode == 0 then
    gameRootPanel:addAnchor(AnchorTop, 'topMenu', AnchorBottom)
    gameLeftPanel:setImageColor('white')
    gameRightPanel:setImageColor('white')
    gameLeftPanel:setMarginTop(0)
    gameRightPanel:setMarginTop(0)
    gameThirdPanel:setMarginTop(0)
    gameForthPanel:setMarginTop(0)
    gameBottomPanel:setImageColor('white')
    modules.client_topmenu.getTopMenu():setImageColor('white')
    
    
    gameThirdPanel:setImageColor('white')
    gameForthPanel:setImageColor('white')      
    gameLeftPanel:setOn(false)
    gameThirdPanel:setOn(false)
    gameForthPanel:setOn(false)
    gameMapPanel:addAnchor(AnchorLeft, 'gameLeftPanel', AnchorRight)
    gameMapPanel:addAnchor(AnchorRight, 'gameRightPanel', AnchorLeft)
    gameMapPanel:addAnchor(AnchorBottom, 'gameBottomPanel', AnchorTop)    

    if panels == 2 then
        gameLeftPanel:setOn(true)
    elseif panels == 3 then
        gameThirdPanel:setOn(true)
        gameMapPanel:addAnchor(AnchorRight, 'gameThirdPanel', AnchorRight)
    elseif panels == 4 then
        gameLeftPanel:setOn(true)
        gameThirdPanel:setOn(true)
        gameMapPanel:addAnchor(AnchorRight, 'gameThirdPanel', AnchorRight)
    elseif panels == 5 then
        gameLeftPanel:setOn(true)
        gameThirdPanel:setOn(true)
        gameForthPanel:setOn(true)
        gameMapPanel:addAnchor(AnchorLeft, 'gameForthPanel', AnchorLeft)
        gameMapPanel:addAnchor(AnchorRight, 'gameThirdPanel', AnchorRight)
    elseif panels == 6 then
        gameLeftPanel:setOn(false)
        gameThirdPanel:setOn(true)
        gameForthPanel:setOn(true)
        gameThirdPanel:setImageColor('alpha')
        gameForthPanel:setImageColor('alpha')
    elseif panels == 7 then
        gameLeftPanel:setOn(true)
        gameThirdPanel:setOn(true)
        gameForthPanel:setOn(true)
        gameThirdPanel:setImageColor('alpha')
        gameForthPanel:setImageColor('alpha')
    end    
    gameMapPanel:setMarginLeft(0)
    gameMapPanel:setMarginRight(0)
    gameBottomPanel:setMarginLeft(0)
    gameBottomPanel:setMarginRight(0)
    gameMapPanel:setKeepAspectRatio(true)
    gameMapPanel:setLimitVisibleRange(false)
    gameMapPanel:setZoom(11)
    gameMapPanel:setVisibleDimension({ width = 15, height = 11 })
    
    healthBar:setMarginTop(0)
    manaBar:setMarginTop(0)

    g_game.changeMapAwareRange(22, 18)
  elseif mode == 1 then
    gameMapPanel:setKeepAspectRatio(false)
    gameMapPanel:setLimitVisibleRange(false)
    gameMapPanel:setZoom(13)
    gameMapPanel:setVisibleDimension({ width = 25, height = 15 })
    gameMapPanel:fill('parent')
    gameRootPanel:fill('parent')
    gameLeftPanel:setImageColor('alpha')
    gameRightPanel:setImageColor('alpha')
    gameThirdPanel:setImageColor('alpha')
    gameForthPanel:setImageColor('alpha')
    gameLeftPanel:setMarginTop(modules.client_topmenu.getTopMenu()
      :getHeight() - gameLeftPanel:getPaddingTop())
    gameRightPanel:setMarginTop(modules.client_topmenu.getTopMenu()
      :getHeight() - gameRightPanel:getPaddingTop())
    gameThirdPanel:setMarginTop(modules.client_topmenu.getTopMenu()
      :getHeight() - gameThirdPanel:getPaddingTop())
    gameForthPanel:setMarginTop(modules.client_topmenu.getTopMenu()
      :getHeight() - gameForthPanel:getPaddingTop())
    gameLeftPanel:setOn(true)
    gameRightPanel:setOn(true)
    gameThirdPanel:setOn(true)
    gameForthPanel:setOn(true)    
    gameBottomPanel:setImageColor('#ffffff88')
    modules.client_topmenu.getTopMenu():setImageColor('#ffffff66')
    
    healthBar:setMarginTop(modules.client_topmenu.getTopMenu():getHeight())
    manaBar:setMarginTop(modules.client_topmenu.getTopMenu():getHeight())
    healthBar:setMarginLeft(200)
    manaBar:setMarginRight(200)

    g_game.changeMapAwareRange(30, 20)
  end
  currentViewMode = mode
end

function limitZoom()
  limitedZoom = true
end

function updateSize() 
  local height = gameMapPanel:getHeight()
  local width = gameMapPanel:getWidth()
   
  local dimenstion = gameMapPanel:getVisibleDimension()
  local dheight = dimenstion.height
  local dwidth = dimenstion.width
  local realWidth = math.floor(math.min(width, height * dwidth / dheight))
  if currentViewMode == 0 then
    healthBar:setMarginLeft(math.max(0, (width - realWidth) / 2 + 2))
    manaBar:setMarginRight(math.max(0, (width - realWidth) / 2 + 2))
  end

  if currentViewMode ~= 1 then
    return
  end
  height = gameRootPanel:getHeight()
  width = gameRootPanel:getWidth()
  if width < 400 or height < 400 then
    gameMapPanel:setMarginLeft(0)
    gameMapPanel:setMarginRight(0)
    gameBottomPanel:setMarginLeft(0)
    gameBottomPanel:setMarginRight(0)
    return
  end
  local maxWidth = math.floor(height * 2)
  local extraMargin = 0
  if width >= maxWidth then
    extraMargin = math.ceil((width - maxWidth) / 2)
  end
  local bottomMaxWidth = 1200  -- something broken, it's not pixels
  local bottomMargin = 0
  if width > bottomMaxWidth then
    bottomMargin = math.ceil((width - bottomMaxWidth) / 2)
  end
  gameMapPanel:setMarginLeft(extraMargin)
  gameMapPanel:setMarginRight(extraMargin)
  gameBottomPanel:setMarginLeft(bottomMargin)
  gameBottomPanel:setMarginRight(bottomMargin)
end
