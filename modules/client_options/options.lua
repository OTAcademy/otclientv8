local defaultOptions = {
  layout = DEFAULT_LAYOUT, -- set in init.lua
  vsync = true,
  showFps = true,
  showPing = true,
  fullscreen = false,
  classicView = not g_app.isMobile(),
  cacheMap = g_app.isMobile(),
  classicControl = not g_app.isMobile(),
  smartWalk = false,
  dash = false,
  autoChaseOverride = true,
  showStatusMessagesInConsole = true,
  showEventMessagesInConsole = true,
  showInfoMessagesInConsole = true,
  showTimestampsInConsole = true,
  showLevelsInConsole = true,
  showPrivateMessagesInConsole = true,
  showPrivateMessagesOnScreen = true,
  rightPanels = 1,
  leftPanels = g_app.isMobile() and 1 or 2,
  containerPanel = 8,
  backgroundFrameRate = 60,
  enableAudio = true,
  enableMusicSound = false,
  musicSoundVolume = 100,
  botSoundVolume = 100,
  enableLights = false,
  floorFading = 500,
  crosshair = 2,
  ambientLight = 100,
  optimizationLevel = 1,
  displayNames = true,
  displayHealth = true,
  displayMana = true,
  displayHealthOnTop = false,
  showHealthManaCircle = false,
  hidePlayerBars = false,
  highlightThingsUnderCursor = true,
  topHealtManaBar = true,
  displayText = true,
  dontStretchShrink = false,
  turnDelay = 30,
  hotkeyDelay = 30,

  chatMode = CHAT_MODE.ON,
  walkFirstStepDelay = 200,
  walkTurnDelay = 100,
  walkStairsDelay = 50,
  walkTeleportDelay = 200,
  walkCtrlTurnDelay = 150,

  topBar = true,

  actionbar1 = true,
  actionbar2 = false,
  actionbar3 = false,
  actionbar4 = false,
  actionbar5 = false,
  actionbar6 = false,
  actionbar7 = false,
  actionbar8 = false,
  actionbar9 = false,

  actionbarLock = false,

  profile = 1,

  antialiasing = true,
  floorShadow = true,

  autoSwitchPreset = true
}

local ActionEdit = {
  EDIT = 1,
  ASSIGN = 2
}

local actionNameLimit = 39

local changedOptions = {}
local changedKeybinds = {}
local changedHotkeys = {}

local optionsWindow
local optionsButton
local optionsTabBar
local options = {}
local extraOptions = {}

local controlsPanel, controlsButton
local keybindsPanel, keybindsButton
local hotkeysButton
local actionSearchEvent
local chatModeGroup
local presetWindow
local keyEditWindow
local assignObjectWindow, assignObjectGroup, mouseGrabber
local assignSpellWindow, assignTextWindow

local interfacePanel, interfaceButton
local hudPanel, hudButton
local consolePanel, consoleButton
local gameWindowPanel, gameWindowButton

local graphicsPanel
local soundPanel
local audioButton

local miscPanel, miscButton
local debugPanel, debugButton

function init()
  for k, v in pairs(defaultOptions) do
    g_settings.setDefault(k, v)
    options[k] = v
  end
  for _, v in ipairs(g_extras.getAll()) do
    extraOptions[v] = g_extras.get(v)
    g_settings.setDefault("extras_" .. v, extraOptions[v])
  end

  optionsWindow = g_ui.displayUI("options")
  optionsWindow:hide()

  optionsTabBar = optionsWindow:getChildById("optionsTabBar")
  optionsTabBar:setContentWidget(optionsWindow:getChildById("optionsTabContent"))

  Keybind.new("UI", "Toggle Fullscreen", "Ctrl+F", "Alt+Enter")
  Keybind.bind("UI", "Toggle Fullscreen", {
    {
      type = KEY_DOWN,
      callback = function() toggleOption("fullscreen") end,
    }
  })

  Keybind.new("UI", "Show/hide Creature Names and Bars", "Ctrl+N", "")
  Keybind.bind("UI", "Show/hide Creature Names and Bars", {
    {
      type = KEY_DOWN,
      callback = toggleDisplays,
    }
  })

  Keybind.new("Dialogs", "Open Options - Custom Hotkeys", "Ctrl+K", "")
  Keybind.bind("Dialogs", "Open Options - Custom Hotkeys", {
    {
      type = KEY_DOWN,
      callback = showHotkeys,
    }
  })

  controlsPanel = g_ui.loadUI("controls")
  controlsButton = optionsTabBar:addTab(tr("Controls"), controlsPanel, "/images/options/icon-controls")

  keybindsPanel = g_ui.loadUI("keybinds")
  chatModeGroup = UIRadioGroup.create()
  chatModeGroup:addWidget(keybindsPanel.chatMode.on)
  chatModeGroup:addWidget(keybindsPanel.chatMode.off)
  chatModeGroup.onSelectionChange = chatModeChange
  chatModeGroup:selectWidget(keybindsPanel.chatMode.on)

  keybindsButton = optionsTabBar:addTab(tr("Keybinds"), keybindsPanel)
  hotkeysButton = optionsTabBar:addTab(tr("Hotkeys"), keybindsPanel)

  addSubTab(controlsButton, keybindsButton, false, function()
    keybindsPanel.search.field:clearText()
    keybindsPanel.buttons.newAction:hide()
    updateKeybinds()
  end)
  addSubTab(controlsButton, hotkeysButton, false, function()
    keybindsPanel.search.field:clearText()
    keybindsPanel.buttons.newAction:show()
    updateHotkeys()
  end)

  assignObjectWindow = g_ui.displayUI("action_object")
  assignObjectWindow.selectObject.button.onClick = showMouseGrabber
  assignObjectWindow.onEnter = assignObject
  assignObjectWindow.onEscape = cancelActionAssignment
  assignObjectWindow.buttons.ok.onClick = assignObject
  assignObjectWindow.buttons.apply.onClick = function() assignObject() end
  assignObjectWindow.buttons.cancel.onClick = cancelActionAssignment
  assignObjectWindow:hide()

  assignObjectGroup = UIRadioGroup.create()
  assignObjectGroup:addWidget(assignObjectWindow.panel.actions.yourself)
  assignObjectGroup:addWidget(assignObjectWindow.panel.actions.target)
  assignObjectGroup:addWidget(assignObjectWindow.panel.actions.crosshair)
  assignObjectGroup:addWidget(assignObjectWindow.panel.actions.equip)
  assignObjectGroup:addWidget(assignObjectWindow.panel.actions.use)
  assignObjectGroup:selectWidget(assignObjectWindow.panel.actions.yourself)

  mouseGrabber = g_ui.createWidget('UIWidget', rootWidget)
  mouseGrabber:setVisible(false)
  mouseGrabber:setFocusable(false)
  mouseGrabber.onMouseRelease = selectObject

  assignSpellWindow = g_ui.displayUI("action_spell")
  assignSpellWindow.onEnter = assignSpell
  assignSpellWindow.onEscape = cancelActionAssignment
  assignSpellWindow.availableSpellsOnly.onCheckChange = filterAssignSpell
  assignSpellWindow.search.field.onTextChange = filterAssignSpell
  assignSpellWindow.buttons.ok.onClick = assignSpell
  assignSpellWindow.buttons.apply.onClick = function() assignSpell() end
  assignSpellWindow.buttons.cancel.onClick = cancelActionAssignment
  assignSpellWindow:hide()
  connect(assignSpellWindow.spells.list, { onChildFocusChange = assingSpellFocusChange })

  assignTextWindow = g_ui.displayUI("action_text")
  assignTextWindow.onEnter = assignText
  assignTextWindow.onEscape = cancelActionAssignment
  assignTextWindow.buttons.ok.onClick = assignText
  assignTextWindow.buttons.apply.onClick = function() assignText() end
  assignTextWindow.buttons.cancel.onClick = cancelActionAssignment
  assignTextWindow:hide()

  keybindsPanel.presets.add.onClick = addNewPreset
  keybindsPanel.presets.copy.onClick = copyPreset
  keybindsPanel.presets.rename.onClick = renamePreset
  keybindsPanel.presets.remove.onClick = removePreset
  keybindsPanel.buttons.newAction:disable()
  keybindsPanel.buttons.newAction.onClick = newHotkeyAction
  keybindsPanel.buttons.reset.onClick = resetActions
  keybindsPanel.search.field.onTextChange = searchActions
  keybindsPanel.search.clear.onClick = function() keybindsPanel.search.field:clearText() end

  presetWindow = g_ui.displayUI("preset")
  presetWindow:hide()

  presetWindow.onEnter = okPresetWindow
  presetWindow.onEscape = cancelPresetWindow
  presetWindow.buttons.ok.onClick = okPresetWindow
  presetWindow.buttons.cancel.onClick = cancelPresetWindow

  keyEditWindow = g_ui.displayUI("key_edit")
  keyEditWindow:hide()

  interfacePanel = g_ui.loadUI("interface")
  interfaceButton = optionsTabBar:addTab(tr("Interface"), interfacePanel, "/images/options/icon-interface")
  interfaceButton:setMarginTop(10)

  hudPanel = g_ui.loadUI("hud")
  hudButton = optionsTabBar:addTab(tr("HUD"), hudPanel)

  consolePanel = g_ui.loadUI("console")
  consoleButton = optionsTabBar:addTab(tr("Console"), consolePanel)

  gameWindowPanel = g_ui.loadUI("game_window")
  gameWindowButton = optionsTabBar:addTab(tr("Game Window"), gameWindowPanel)

  addSubTab(interfaceButton, hudButton, true)
  addSubTab(interfaceButton, consoleButton, true)
  addSubTab(interfaceButton, gameWindowButton, true)

  graphicsPanel = g_ui.loadUI("graphics")
  optionsTabBar:addTab(tr("Graphics"), graphicsPanel, "/images/options/icon-graphics"):setMarginTop(10)

  soundPanel = g_ui.loadUI("audio")
  optionsTabBar:addTab(tr("Sound"), soundPanel, "/images/options/icon-sound"):setMarginTop(10)

  miscPanel = g_ui.loadUI("misc")
  miscButton = optionsTabBar:addTab(tr("Misc"), miscPanel, "/images/options/icon-misc")
  miscButton:setMarginTop(10)

  if not g_game.getFeature(GameNoDebug) and not g_app.isMobile() then
    debugPanel = g_ui.loadUI("debug")
    debugButton = optionsTabBar:addTab(tr("Debug"), debugPanel)

    addSubTab(miscButton, debugButton, true)

    for _, v in ipairs(g_extras.getAll()) do
      local extrasButton = g_ui.createWidget("OptionCheckBox")
      extrasButton:setId(v)
      extrasButton:setText(g_extras.getDescription(v))
      debugPanel:addChild(extrasButton)
    end
  end

  optionsButton = modules.client_topmenu.addLeftButton("optionsButton", tr("Options"), "/images/topbuttons/options", toggle)
  audioButton = modules.client_topmenu.addLeftButton("audioButton", tr("Audio"), "/images/topbuttons/audio", function() toggleOption("enableAudio") end)
  if g_app.isMobile() then
    audioButton:hide()
  end

  addEvent(function() setup() end)

  connect(g_game, {
    onGameStart = online,
    onGameEnd = offline
  })
end

function terminate()
  disconnect(g_game, {
    onGameStart = online,
    onGameEnd = offline
  })

  Keybind.delete("UI", "Toggle Fullscreen")
  Keybind.delete("UI", "Show/hide Creature Names and Bars")
  Keybind.delete("Dialogs", "Open Options - Custom Hotkeys")

  if optionsWindow then
    optionsWindow:destroy()
    optionsWindow = nil
  end

  if optionsButton then
    optionsButton:destroy()
    optionsButton = nil
  end

  if audioButton then
    audioButton:destroy()
    audioButton = nil
  end

  if presetWindow then
    presetWindow:destroy()
    presetWindow = nil
  end

  if chatModeGroup then
    chatModeGroup:destroy()
    chatModeGroup = nil
  end

  if keyEditWindow then
    if keyEditWindow:isVisible() then
      keyEditWindow:ungrabKeyboard()
      disconnect(keyEditWindow, { onKeyDown = editKeybindKeyDown })
    end
    keyEditWindow:destroy()
    keyEditWindow = nil
  end

  if assignObjectWindow then
    assignObjectGroup:destroy()
    if mouseGrabber:isVisible() then
      mouseGrabber:ungrabMouse()
    end
    mouseGrabber:destroy()
    assignObjectWindow:destroy()
    assignObjectWindow = nil
  end

  if assignSpellWindow then
    disconnect(assignSpellWindow.spells.list, { onChildFocusChange = assingSpellFocusChange })

    assignSpellWindow:destroy()
    assignSpellWindow = nil
  end

  if assignTextWindow then
    assignTextWindow:destroy()
    assignTextWindow = nil
  end

  actionSearchEvent = nil
end

function setup()
  optionsTabBar.onTabChange = onTabChange

  for _, preset in ipairs(Keybind.presets) do
    keybindsPanel.presets.list:addOption(preset)
  end

  keybindsPanel.presets.list.onOptionChange = function(widget, option)
    presetOption(widget, "currentPreset", option, false)

    changedKeybinds = {}
    changedHotkeys = {}

    if optionsTabBar.currentTab == keybindsButton then
      updateKeybinds()
    elseif optionsTabBar.currentTab == hotkeysButton then
      updateHotkeys()
    end
  end
  keybindsPanel.presets.list:setCurrentOption(Keybind.currentPreset)

  local spells = SpelllistSettings["Default"].spellOrder
  for _, spellName in ipairs(spells) do
    local spellData = SpellInfo["Default"][spellName]
    local spellWidget = g_ui.createWidget("HotkeySpellAction", assignSpellWindow.spells.list)
    spellWidget:setId(spellData.words)
    spellWidget.spellName = spellName
    spellWidget.words = spellData.words
    spellWidget.parameter = spellData.parameter
    spellWidget:setImageClip(Spells.getImageClip(SpellIcons[spellData.icon][1], "Default"))
    spellWidget:setText(string.format("%s\n%s", spellName, spellData.words))
  end

  assignSpellWindow.spells.list:focusChild(assignSpellWindow.spells.list:getChildByIndex(1))

  -- load options
  for k, v in pairs(defaultOptions) do
    if type(v) == "boolean" then
      setOption(k, g_settings.getBoolean(k), true)
    elseif type(v) == "number" then
      setOption(k, g_settings.getNumber(k), true)
    elseif type(v) == "string" then
      setOption(k, g_settings.getString(k), true)
    end
  end

  for _, v in ipairs(g_extras.getAll()) do
    g_extras.set(v, g_settings.getBoolean("extras_" .. v))
    local widget = debugPanel:recursiveGetChildById(v)
    if widget then
      widget:setChecked(g_extras.get(v))
    end
  end

  assignSpellWindow.availableSpellsOnly:setChecked(false)

  if g_game.isOnline() then
    online()
  end
end

function toggle()
  if keyEditWindow:isVisible() or presetWindow:isVisible() then
    return
  end

  if optionsWindow:isVisible() then
    hide()
  else
    show()
  end
end

function show()
  if keyEditWindow:isVisible() or presetWindow:isVisible() then
    return
  end

  optionsWindow:show()
  optionsWindow:raise()
  optionsWindow:focus()
end

function hide()
  optionsWindow:hide()
end

function showHotkeys()
  if not optionsWindow:isVisible() then
    optionsTabBar:selectTab(hotkeysButton)
    show()
  end
end

function addSubTab(parent, subTab, hidden, callback)
  if not parent.subTabs then
    parent.subTabs = {}
  end

  parent.subTabs[subTab:getId()] = subTab
  subTab.subParent = parent
  subTab.arrow:hide()
  if hidden then
    subTab:hide()
  end
  subTab.tabCallback = callback
end

function onTabChange(tabBar, tab)
  tab.arrow:setOn(true)

  for _, t in ipairs(tabBar:getTabs()) do
    if t.subParent then
      if tab.tabPanel ~= t.tabPanel and tab.subParent ~= t.subParent then
        t:hide()
      end
      t.arrow:hide()
    elseif t.subTabs then
      t.arrow:show()
    end
  end

  if tab.subParent then
    tab.subParent.arrow:hide()
    for _, subTab in pairs(tab.subParent.subTabs) do
      subTab.arrow:hide()
    end
    tab:show()
    tab.arrow:show()

    if tab.tabCallback then
      tab.tabCallback()
    end
  elseif tab.subTabs then
    tab.arrow:show()
    for _, subTab in pairs(tab.subTabs) do
      subTab:show()
      subTab.arrow:hide()
    end
  end

  if tabBar.currentTab then
    tabBar.currentTab.arrow:setOn(false)
  end
end

function toggleDisplays()
  if options["displayNames"] and options["displayHealth"] and options["displayMana"] then
    setOption("displayNames", false)
  elseif options["displayHealth"] then
    setOption("displayHealth", false)
    setOption("displayMana", false)
  else
    if not options["displayNames"] and not options["displayHealth"] then
      setOption("displayNames", true)
    else
      setOption("displayHealth", true)
      setOption("displayMana", true)
    end
  end
end

function toggleOption(key)
  setOption(key, not getOption(key))
end

function presetOption(widget, key, value, force)
  if not optionsWindow:isVisible() then
    return
  end

  changedOptions[key] = { widget = widget, value = value, force = force }

  updateValues(key, value)
end

function updateValues(key, value)
  local gameMapPanel = modules.game_interface.getMapPanel()

  if key == "vsync" then
    g_window.setVerticalSync(value)
  elseif key == "showFps" then
    modules.client_topmenu.setFpsVisible(value)
    if modules.game_stats and modules.game_stats.ui.fps then
      modules.game_stats.ui.fps:setVisible(value)
    end
  elseif key == "showPing" then
    modules.client_topmenu.setPingVisible(value)
    if modules.game_stats and modules.game_stats.ui.ping then
      modules.game_stats.ui.ping:setVisible(value)
    end
  elseif key == "fullscreen" then
    g_window.setFullscreen(value)
  elseif key == "enableAudio" then
    if g_sounds ~= nil then
      g_sounds.setAudioEnabled(value)
    end
    if value then
      audioButton:setIcon("/images/topbuttons/audio")
    else
      audioButton:setIcon("/images/topbuttons/audio_mute")
    end
  elseif key == "enableMusicSound" then
    if g_sounds ~= nil then
      g_sounds.getChannel(SoundChannels.Music):setEnabled(value)
    end
  elseif key == "musicSoundVolume" then
    if g_sounds ~= nil then
      g_sounds.getChannel(SoundChannels.Music):setGain(value / 100)
    end
    soundPanel:getChildById("musicSoundVolumeLabel"):setText(tr("Music volume: %d", value))
  elseif key == "botSoundVolume" then
    if g_sounds ~= nil then
      g_sounds.getChannel(SoundChannels.Bot):setGain(value / 100)
    end
    soundPanel:getChildById("botSoundVolumeLabel"):setText(tr("Bot sound volume: %d", value))
  elseif key == "showHealthManaCircle" then
    modules.game_healthinfo.healthCircle:setVisible(value)
    modules.game_healthinfo.healthCircleFront:setVisible(value)
    modules.game_healthinfo.manaCircle:setVisible(value)
    modules.game_healthinfo.manaCircleFront:setVisible(value)
  elseif key == "backgroundFrameRate" then
    local text, v = value, value
    if value <= 0 or value >= 201 then
      text = "max"
      v = 0
    end
    graphicsPanel:getChildById("backgroundFrameRateLabel"):setText(tr("Game framerate limit: %s", text))
    g_app.setMaxFps(v)
  elseif key == "enableLights" then
    gameMapPanel:setDrawLights(value and options["ambientLight"] < 100)
    graphicsPanel:getChildById("ambientLight"):setEnabled(value)
    graphicsPanel:getChildById("ambientLightLabel"):setEnabled(value)
  elseif key == "floorFading" then
    gameMapPanel:setFloorFading(value)
    gameWindowPanel:getChildById("floorFadingLabel"):setText(tr("Floor fading: %s ms", value))
  elseif key == "crosshair" then
    if value == 1 then
      gameMapPanel:setCrosshair("")
    elseif value == 2 then
      gameMapPanel:setCrosshair("/images/crosshair/default.png")
    elseif value == 3 then
      gameMapPanel:setCrosshair("/images/crosshair/full.png")
    end
  elseif key == "ambientLight" then
    graphicsPanel:getChildById("ambientLightLabel"):setText(tr("Ambient light: %s%%", value))
    gameMapPanel:setMinimumAmbientLight(value / 100)
    gameMapPanel:setDrawLights(options["enableLights"] and value < 100)
  elseif key == "optimizationLevel" then
    g_adaptiveRenderer.setLevel(value - 2)
  elseif key == "displayNames" then
    gameMapPanel:setDrawNames(value)
  elseif key == "displayHealth" then
    gameMapPanel:setDrawHealthBars(value)
  elseif key == "displayMana" then
    gameMapPanel:setDrawManaBar(value)
  elseif key == "displayHealthOnTop" then
    gameMapPanel:setDrawHealthBarsOnTop(value)
  elseif key == "hidePlayerBars" then
    gameMapPanel:setDrawPlayerBars(value)
  elseif key == "topHealtManaBar" then
    modules.game_healthinfo.topHealthBar:setVisible(value)
    modules.game_healthinfo.topManaBar:setVisible(value)
  elseif key == "displayText" then
    gameMapPanel:setDrawTexts(value)
  elseif key == "dontStretchShrink" then
    addEvent(function()
      modules.game_interface.updateStretchShrink()
    end)
  elseif key == "dash" then
    if value then
      g_game.setMaxPreWalkingSteps(2)
    else
      g_game.setMaxPreWalkingSteps(1)
    end
  elseif key == "hotkeyDelay" then
    controlsPanel:getChildById("hotkeyDelayLabel"):setText(tr("Hotkey delay: %s ms", value))
  elseif key == "walkFirstStepDelay" then
    controlsPanel:getChildById("walkFirstStepDelayLabel"):setText(tr("Walk delay after first step: %s ms", value))
  elseif key == "walkTurnDelay" then
    controlsPanel:getChildById("walkTurnDelayLabel"):setText(tr("Walk delay after turn: %s ms", value))
  elseif key == "walkStairsDelay" then
    controlsPanel:getChildById("walkStairsDelayLabel"):setText(tr("Walk delay after floor change: %s ms", value))
  elseif key == "walkTeleportDelay" then
    controlsPanel:getChildById("walkTeleportDelayLabel"):setText(tr("Walk delay after teleport: %s ms", value))
  elseif key == "walkCtrlTurnDelay" then
    controlsPanel:getChildById("walkCtrlTurnDelayLabel"):setText(tr("Walk delay after ctrl turn: %s ms", value))
  elseif key == "antialiasing" then
    g_app.setSmooth(value)
  elseif key == "floorShadow" then
    if value then
      g_game.enableFeature(GameDrawFloorShadow)
    else
      g_game.disableFeature(GameDrawFloorShadow)
    end
  elseif key == "chatMode" then
    Keybind.setChatMode(value)
    local check = value ~= CHAT_MODE.ON and true or false
    if modules.game_console and modules.game_console.consoleToggleChat:isChecked() ~= check then
      modules.game_console.consoleToggleChat:setChecked(check)
    end
  end
end

function setOption(key, value, force)
  if extraOptions[key] ~= nil then
    g_extras.set(key, value)
    g_settings.set("extras_" .. key, value)
    if key == "debugProxy" and modules.game_proxy then
      if value then
        modules.game_proxy.show()
      else
        modules.game_proxy.hide()
      end
    end
    updateValues(key, value)
    return
  end

  if modules.game_interface == nil then
    return
  end

  if not force and options[key] == value then return end

  updateValues(key, value)

  for _, panel in pairs(optionsTabBar:getTabsPanel()) do
    local widget = panel:recursiveGetChildById(key)
    if widget then
      if widget:getStyle().__class == "UICheckBox" then
        widget:setChecked(value)
      elseif widget:getStyleName() == "OptionCheckBox" then
        widget:setChecked(value)
      elseif widget:getStyle().__class == "UIScrollBar" then
        widget:setValue(value)
      elseif widget:getStyle().__class == "UIComboBox" then
        if type(value) == "string" then
          widget:setCurrentOption(value, true)
          break
        end
        if value == nil or value < 1 then
          value = 1
        end
        if widget.currentIndex ~= value then
          widget:setCurrentIndex(value, true)
        end
      end
      break
    end
  end

  if key == "currentPreset" then
    Keybind.selectPreset(value)
    keybindsPanel.presets.list:setCurrentOption(value, true)
  end

  g_settings.set(key, value)
  options[key] = value

  if key == "profile" then
    modules.client_profiles.onProfileChange()
  end

  if key == "classicView" or key == "rightPanels" or key == "leftPanels" or key == "cacheMap" then
    modules.game_interface.refreshViewMode()
  elseif key:find("actionbar") then
    modules.game_actionbar.show()
  end

  if key == "topBar" then
    modules.game_topbar.show()
  end
end

function getOption(key)
  return options[key]
end

function applyChangedOptions()
  local needKeybindsUpdate = false
  local needHotkeysUpdate = false

  for key, option in pairs(changedOptions) do
    if key == "resetKeybinds" then
      Keybind.resetKeybindsToDefault(option.value, option.chatMode)
      if optionsTabBar.currentTab == keybindsButton then
        needKeybindsUpdate = true
      end
    elseif key == "resetHotkeys" then
      Keybind.removeAllHotkeys(option.chatMode)
      if optionsTabBar.currentTab == hotkeysButton then
        needHotkeysUpdate = true
      end
    else
      setOption(key, option.value, option.force)
    end
  end
  changedOptions = {}

  for preset, keybinds in pairs(changedKeybinds) do
    for index, keybind in pairs(keybinds) do
      if keybind.primary then
        if Keybind.setPrimaryActionKey(keybind.primary.category, keybind.primary.action, preset, keybind.primary.keyCombo, getChatMode()) then
          if optionsTabBar.currentTab == keybindsButton then
            needKeybindsUpdate = true
          end
        end
      elseif keybind.secondary then
        if Keybind.setSecondaryActionKey(keybind.secondary.category, keybind.secondary.action, preset, keybind.secondary.keyCombo, getChatMode()) then
          if optionsTabBar.currentTab == keybindsButton then
            needKeybindsUpdate = true
          end
        end
      end
    end
  end
  changedKeybinds = {}

  for _, hotkey in ipairs(changedHotkeys) do
    if hotkey.new then
      if hotkey.primary and hotkey.primary:len() > 0 or hotkey.secondary and hotkey.secondary:len() > 0 then
        Keybind.newHotkey(hotkey.action, hotkey.data, hotkey.primary, hotkey.secondary, getChatMode())
      end
      needHotkeysUpdate = true
    end

    if hotkey.edit then
      Keybind.editHotkey(hotkey.hotkeyId, hotkey.action, hotkey.data, getChatMode())
      needHotkeysUpdate = true
    end

    if hotkey.editKey then
      if (not hotkey.primary and not hotkey.secondary) or (hotkey.primary and hotkey.primary:len() == 0 and hotkey.secondary and hotkey.secondary:len() == 0) then
        Keybind.removeHotkey(hotkey.hotkeyId, getChatMode())
      else
        Keybind.editHotkeyKeys(hotkey.hotkeyId, hotkey.primary, hotkey.secondary, getChatMode())
      end
      needHotkeysUpdate = true
    end

    if hotkey.remove then
      Keybind.removeHotkey(hotkey.hotkeyId, getChatMode())
      needHotkeysUpdate = true
    elseif not hotkey.new and not hotkey.edit and not hotkey.editKey then
      if (not hotkey.primary and not hotkey.secondary) or (hotkey.primary:len() > 0 and hotkey.secondary:len() > 0) then
        Keybind.removeHotkey(hotkey.hotkeyId, getChatMode())
        needHotkeysUpdate = true
      end
    end
  end
  changedHotkeys = {}

  if needKeybindsUpdate then
    updateKeybinds()
  end

  if needHotkeysUpdate then
    updateHotkeys()
  end

  g_settings.save()
end

-- hide/show
function online()
  keybindsPanel.buttons.newAction:enable()

  setLightOptionsVisibility(not g_game.getFeature(GameForceLight))

  g_app.setSmooth(g_settings.getBoolean("antialiasing"))

  if g_settings.getBoolean("autoSwitchPreset") then
    local name = g_game.getCharacterName()
    if Keybind.selectPreset(name) then
      keybindsPanel.presets.list:setCurrentOption(name, true)

      if optionsTabBar.currentTab == keybindsButton then
        updateKeybinds()
      elseif optionsTabBar.currentTab == hotkeysButton then
        updateHotkeys()
      end
    end
  end

  addEvent(function()
    assignSpellWindow.availableSpellsOnly:setChecked(true)
  end)
end

function offline()
  keybindsPanel.buttons.newAction:disable()
  setLightOptionsVisibility(true)
end

function okButton()
  applyChangedOptions()

  toggle()
end

function applyButton()
  applyChangedOptions()
end

function cancelButton()
  toggle()

  if changedOptions["resetKeybinds"] then
    changedOptions["resetKeybinds"] = nil
    updateKeybinds()
  end
  if changedOptions["resetHotkeys"] then
    changedOptions["resetHotkeys"] = nil
    updateHotkeys()
  end

  for key, option in pairs(changedOptions) do
    local widget = option.widget
    local value = options[key]

    updateValues(key, value)

    if key == "currentPreset" then
      keybindsPanel.presets.list:setCurrentOption(Keybind.currentPreset)
    elseif widget then
      if widget:getStyle().__class == "UICheckBox" then
        widget:setChecked(value)
      elseif widget:getStyleName() == "OptionCheckBox" then
        widget:setChecked(value)
      elseif widget:getStyle().__class == "UIScrollBar" then
        widget:setValue(value)
      elseif widget:getStyle().__class == "UIComboBox" then
        if type(value) == "string" then
          widget:setCurrentOption(value, true)
          break
        end
        if value == nil or value < 1 then
          value = 1
        end
        if widget.currentIndex ~= value then
          widget:setCurrentIndex(value, true)
        end
      end
    end
  end

  changedOptions = {}

  if optionsTabBar.currentTab == keybindsButton then
    if next(changedKeybinds) then
      updateKeybinds()
    end
  elseif optionsTabBar.currentTab == hotkeysButton then
    if #changedHotkeys > 0 then
      updateHotkeys()
    end
  end

  changedKeybinds = {}
  changedHotkeys = {}
end

-- classic view

-- graphics
function setLightOptionsVisibility(value)
  graphicsPanel:getChildById("enableLights"):setEnabled(value)
  graphicsPanel:getChildById("ambientLightLabel"):setEnabled(value)
  graphicsPanel:getChildById("ambientLight"):setEnabled(value)
  gameWindowPanel:getChildById("floorFading"):setEnabled(value)
  gameWindowPanel:getChildById("floorFadingLabel"):setEnabled(value)
  gameWindowPanel:getChildById("floorFadingLabel2"):setEnabled(value)
end

-- controls and keybinds

function addNewPreset()
  presetWindow:setText(tr("Add hotkey preset"))

  presetWindow.info:setText(tr("Enter a name for the new preset:"))

  presetWindow.field:clearText()
  presetWindow.field:show()
  presetWindow.field:focus()

  presetWindow:setWidth(360)

  presetWindow.action = "add"

  presetWindow:show()
  presetWindow:raise()
  presetWindow:focus()

  optionsWindow:hide()
end

function copyPreset()
  presetWindow:setText(tr("Copy hotkey preset"))

  presetWindow.info:setText(tr("Enter a name for the new preset:"))

  presetWindow.field:clearText()
  presetWindow.field:show()
  presetWindow.field:focus()

  presetWindow.action = "copy"

  presetWindow:setWidth(360)
  presetWindow:show()
  presetWindow:raise()
  presetWindow:focus()

  optionsWindow:hide()
end

function renamePreset()
  presetWindow:setText(tr("Rename hotkey preset"))

  presetWindow.info:setText(tr("Enter a name for the preset:"))

  presetWindow.field:setText(keybindsPanel.presets.list:getCurrentOption().text)
  presetWindow.field:setCursorPos(1000)
  presetWindow.field:show()
  presetWindow.field:focus()

  presetWindow.action = "rename"

  presetWindow:setWidth(360)
  presetWindow:show()
  presetWindow:raise()
  presetWindow:focus()

  optionsWindow:hide()
end

function removePreset()
  presetWindow:setText(tr("Warning"))

  presetWindow.info:setText(tr("Do you really want to delete the hotkey preset %s?", keybindsPanel.presets.list:getCurrentOption().text))
  presetWindow.field:hide()
  presetWindow.action = "remove"

  presetWindow:setWidth(presetWindow.info:getTextSize().width + presetWindow:getPaddingLeft() +
    presetWindow:getPaddingRight())
  presetWindow:show()
  presetWindow:raise()
  presetWindow:focus()

  optionsWindow:hide()
end

function okPresetWindow()
  local presetName = presetWindow.field:getText():trim()
  local selectedPreset = keybindsPanel.presets.list:getCurrentOption().text

  presetWindow:hide()
  show()

  if presetWindow.action == "add" then
    Keybind.newPreset(presetName)
    keybindsPanel.presets.list:addOption(presetName)
    keybindsPanel.presets.list:setCurrentOption(presetName)
  elseif presetWindow.action == "copy" then
    if not Keybind.copyPreset(selectedPreset, presetName) then
      return
    end

    keybindsPanel.presets.list:addOption(presetName)
    keybindsPanel.presets.list:setCurrentOption(presetName)
  elseif presetWindow.action == "rename" then
    if selectedPreset ~= presetName then
      keybindsPanel.presets.list:updateCurrentOption(presetName)
      if changedOptions["currentPreset"] then
        changedOptions["currentPreset"].value = presetName
      end
      Keybind.renamePreset(selectedPreset, presetName)
    end
  elseif presetWindow.action == "remove" then
    if Keybind.removePreset(selectedPreset) then
      keybindsPanel.presets.list:removeOption(selectedPreset)
    end
  end
end

function cancelPresetWindow()
  presetWindow:hide()
  show()
end

function editKeybindKeyDown(widget, keyCode, keyboardModifiers)
  keyEditWindow.keyCombo:setText(determineKeyComboDesc(keyCode, keyEditWindow.alone:isVisible() and KeyboardNoModifier or keyboardModifiers))

  local category = nil
  local action = nil

  if keyEditWindow.keybind then
    category = keyEditWindow.keybind.category
    action = keyEditWindow.keybind.action
  end

  local keyCombo = keyEditWindow.keyCombo:getText()
  local keyUsed = Keybind.isKeyComboUsed(keyCombo, category, action, getChatMode())
  if not keyUsed then
    for _, change in ipairs(changedHotkeys) do
      if change.primary == keyCombo or change.secondary == keyCombo then
        keyUsed = true
        break
      end
    end
  end

  keyEditWindow.buttons.ok:setEnabled(not keyUsed)
  keyEditWindow.used:setVisible(keyUsed)
end

function editKeybind(keybind)
  keyEditWindow.buttons.cancel.onClick = function()
    disconnect(keyEditWindow, { onKeyDown = editKeybindKeyDown })
    keyEditWindow:hide()
    keyEditWindow:ungrabKeyboard()
    show()
  end

  keyEditWindow.info:setText(tr("Click \"Ok\" to assign the keybind. Click \"Clear\" to remove the keybind from \"%s: %s\".", keybind.category, keybind.action))
  keyEditWindow.alone:setVisible(keybind.alone)

  connect(keyEditWindow, { onKeyDown = editKeybindKeyDown })

  keyEditWindow:show()
  keyEditWindow:raise()
  keyEditWindow:focus()
  keyEditWindow:grabKeyboard()
  hide()
end

function editKeybindPrimary(button)
  local column = button:getParent()
  local row = column:getParent()
  local index = row.category .. "_" .. row.action
  local keybind = Keybind.getAction(row.category, row.action)
  local preset = keybindsPanel.presets.list:getCurrentOption().text

  keyEditWindow.keybind = { category = row.category, action = row.action }

  keyEditWindow:setText(tr("Edit Primary Key for \"%s\"", string.format("%s: %s", keybind.category, keybind.action)))
  keyEditWindow.keyCombo:setText(Keybind.getKeybindKeys(row.category, row.action, getChatMode(), preset).primary)

  editKeybind(keybind)

  keyEditWindow.buttons.ok.onClick = function()
    local keyCombo = keyEditWindow.keyCombo:getText()

    column:setText(keyEditWindow.keyCombo:getText())

    if not changedKeybinds[preset] then
      changedKeybinds[preset] = {}
    end
    if not changedKeybinds[preset][index] then
      changedKeybinds[preset][index] = {}
    end
    changedKeybinds[preset][index].primary = {
      category = row.category,
      action = row.action,
      keyCombo = keyCombo
    }

    disconnect(keyEditWindow, { onKeyDown = editKeybindKeyDown })
    keyEditWindow:hide()
    keyEditWindow:ungrabKeyboard()
    show()
  end

  keyEditWindow.buttons.clear.onClick = function()
    if not changedKeybinds[preset] then
      changedKeybinds[preset] = {}
    end
    if not changedKeybinds[preset][index] then
      changedKeybinds[preset][index] = {}
    end
    changedKeybinds[preset][index].primary = {
      category = row.category,
      action = row.action,
      keyCombo = ""
    }

    column:setText("")

    disconnect(keyEditWindow, { onKeyDown = editKeybindKeyDown })
    keyEditWindow:hide()
    keyEditWindow:ungrabKeyboard()
    show()
  end
end

function editKeybindSecondary(button)
  local column = button:getParent()
  local row = column:getParent()
  local index = row.category .. "_" .. row.action
  local keybind = Keybind.getAction(row.category, row.action)
  local preset = keybindsPanel.presets.list:getCurrentOption().text

  keyEditWindow.keybind = { category = row.category, action = row.action }

  keyEditWindow:setText(tr("Edit Secondary Key for \"%s\"", string.format("%s: %s", keybind.category, keybind.action)))
  keyEditWindow.keyCombo:setText(Keybind.getKeybindKeys(row.category, row.action, getChatMode(), preset).secondary)

  editKeybind(keybind)

  keyEditWindow.buttons.ok.onClick = function()
    local keyCombo = keyEditWindow.keyCombo:getText()

    column:setText(keyEditWindow.keyCombo:getText())

    if not changedKeybinds[preset] then
      changedKeybinds[preset] = {}
    end
    if not changedKeybinds[preset][index] then
      changedKeybinds[preset][index] = {}
    end
    changedKeybinds[preset][index].secondary = {
      category = row.category,
      action = row.action,
      keyCombo = keyCombo
    }

    disconnect(keyEditWindow, { onKeyDown = editKeybindKeyDown })
    keyEditWindow:hide()
    keyEditWindow:ungrabKeyboard()
    show()
  end

  keyEditWindow.buttons.clear.onClick = function()
    if not changedKeybinds[preset] then
      changedKeybinds[preset] = {}
    end
    if not changedKeybinds[preset][index] then
      changedKeybinds[preset][index] = {}
    end
    changedKeybinds[preset][index].secondary = {
      category = row.category,
      action = row.action,
      keyCombo = ""
    }

    column:setText("")

    disconnect(keyEditWindow, { onKeyDown = editKeybindKeyDown })
    keyEditWindow:hide()
    keyEditWindow:ungrabKeyboard()
    show()
  end
end

function resetActions()
  if optionsTabBar.currentTab == keybindsButton then
    changedOptions["resetKeybinds"] = { value = keybindsPanel.presets.list:getCurrentOption().text }
    updateKeybinds()
  elseif optionsTabBar.currentTab == hotkeysButton then
    changedOptions["resetHotkeys"] = { value = keybindsPanel.presets.list:getCurrentOption().text, chatMode = getChatMode() }
    keybindsPanel.tablePanel.keybinds:clearData()
  end
end

function updateKeybinds()
  keybindsPanel.tablePanel.keybinds:clearData()

  local sortedKeybinds = {}

  for index, _ in pairs(Keybind.defaultKeybinds) do
    table.insert(sortedKeybinds, index)
  end

  table.sort(sortedKeybinds, function(a, b)
    local keybindA = Keybind.defaultKeybinds[a]
    local keybindB = Keybind.defaultKeybinds[b]

    if keybindA.category ~= keybindB.category then
      return keybindA.category < keybindB.category
    end
    return keybindA.action < keybindB.action
  end)

  local preset = keybindsPanel.presets.list:getCurrentOption().text
  for _, index in ipairs(sortedKeybinds) do
    local keybind = Keybind.defaultKeybinds[index]
    local keys = Keybind.getKeybindKeys(keybind.category, keybind.action, getChatMode(), preset, changedOptions["resetKeybinds"])
    addKeybind(keybind.category, keybind.action, keys.primary, keys.secondary)
  end
end

function updateHotkeys()
  keybindsPanel.tablePanel.keybinds:clearData()

  local chatMode = getChatMode()
  local preset = keybindsPanel.presets.list:getCurrentOption().text
  if Keybind.hotkeys[chatMode][preset] then
    for _, hotkey in ipairs(Keybind.hotkeys[chatMode][preset]) do
      addHotkey(hotkey.hotkeyId, hotkey.action, hotkey.data, hotkey.primary, hotkey.secondary)
    end
  end
end

function newHotkeyAction()
  local mousePos = g_window.getMousePosition()
  local menu = g_ui.createWidget("PopupMenu")
  menu:setGameMenu(true)

  menu:addOption(tr("Assign Spell"), openAssignSpell)
  menu:addOption(tr("Assign Object"), openAssignObject)
  menu:addOption(tr("Assign Text"), openAssignText)

  menu:display(mousePos)
end

function assignSpell(ok)
  local parameter = nil

  if assignSpellWindow.selectedSpell.parameter then
    parameter = assignSpellWindow.parameter.field:getText():trim()
  end

  local data = { spellName = assignSpellWindow.selectedSpell.spellName, words = assignSpellWindow.selectedSpell.words, parameter = parameter }
  local action = HOTKEY_ACTION.SPELL

  if assignSpellWindow.actionEdit then
    assignSpellWindow.row.actionData = data
    assignSpellWindow.row.action = action

    local text = data.words
    if data.parameter then
      text = text .. " " .. data.parameter
    end

    local firstColumn = assignSpellWindow.row:getChildByIndex(1)

    if assignSpellWindow.actionEdit == ActionEdit.EDIT then
      firstColumn:setText(text)
    elseif assignSpellWindow.actionEdit == ActionEdit.ASSIGN then
      firstColumn:destroy()
      firstColumn = g_ui.createWidget("EditableKeybindsTableColumn")
      assignSpellWindow.row:insertChild(1, firstColumn)
      firstColumn:setWidth(286)
      firstColumn:setText(text)
      firstColumn:setColor("#ffffff")
      firstColumn.edit.onClick = editHotkeyAction
      firstColumn.edit:show()
      assignSpellWindow.row:getChildByIndex(3):setColor("#ffffff")
      assignSpellWindow.row:getChildByIndex(5):setColor("#ffffff")
    end
    local change = table.findbyfield(changedHotkeys, "hotkeyId", assignSpellWindow.row.hotkeyId)
    if change then
      change.action = action
      change.edit = true
      change.data = data
    else
      table.insert(changedHotkeys, { hotkeyId = assignSpellWindow.row.hotkeyId, data = data, action = action, edit = true })
    end
  else
    preAddHotkey(action, data)
  end

  if ok then
    assignSpellWindow:hide()
    optionsWindow:show()
  end
end

function assingSpellFocusChange(widget, focusedChild)
  local spellWidget = assignSpellWindow.selectedSpell
  spellWidget.spellName = focusedChild.spellName
  spellWidget.words = focusedChild.words
  spellWidget.parameter = focusedChild.parameter
  spellWidget:setImageClip(Spells.getImageClip(SpellIcons[SpellInfo["Default"][focusedChild.spellName].icon][1], "Default"))
  spellWidget:setText(focusedChild:getText())

  assignSpellWindow.parameter.label:setEnabled(focusedChild.parameter)
  assignSpellWindow.parameter.field:setEnabled(focusedChild.parameter)
  local param = (focusedChild.parameter and type(focusedChild.parameter) == "string") and focusedChild.parameter or ""
  assignSpellWindow.parameter.field:setPlaceholder(param)
end

function openAssignSpell(position, actionEdit, row)
  if actionEdit and actionEdit == ActionEdit.EDIT then
    local selectedSpell = assignSpellWindow.spells.list[row.actionData.words]
    if not selectedSpell:isVisible() then
      assignSpellWindow.availableSpellsOnly.checkBox:setChecked(false)
    end
    assignSpellWindow.spells.list:focusChild(selectedSpell)
  end

  assignSpellWindow.actionEdit = actionEdit
  assignSpellWindow.row = row

  assignSpellWindow:show()
  assignSpellWindow:raise()
  assignSpellWindow:focus()

  optionsWindow:hide()
end

function showMouseGrabber()
  assignObjectWindow:hide()

  mouseGrabber:grabMouse()
  g_mouse.pushCursor('target')
end

function assignObject(ok)
  if not assignObjectWindow.panel.item:getItem() then
    return
  end

  local itemId = assignObjectWindow.panel.item:getItemId()
  if itemId == 0 then
    return
  end

  local action = HOTKEY_ACTION.USE
  local data = { itemId = itemId, itemSubType = assignObjectWindow.panel.item:getItemSubType() }

  if assignObjectWindow.panel.actions.yourself:isChecked() then
    action = HOTKEY_ACTION.USE_YOURSELF
  elseif assignObjectWindow.panel.actions.target:isChecked() then
    action = HOTKEY_ACTION.USE_TARGET
  elseif assignObjectWindow.panel.actions.crosshair:isChecked() then
    action = HOTKEY_ACTION.USE_CROSSHAIR
  elseif assignObjectWindow.panel.actions.equip:isChecked() then
    action = HOTKEY_ACTION.EQUIP
  end

  if assignObjectWindow.actionEdit then
    assignObjectWindow.row.actionData = data
    assignObjectWindow.row.action = action
    local firstColumn = assignObjectWindow.row:getChildByIndex(1)
    if assignObjectWindow.actionEdit == ActionEdit.EDIT then
      updateHotkeyItem(firstColumn.item, data.itemId, data.itemSubType)
      if action == HOTKEY_ACTION.USE_YOURSELF then
        firstColumn:setText(tr("(use object on yourself)"))
        firstColumn:setColor("#b0ffb0")
        assignObjectWindow.row:getChildByIndex(3):setColor("#b0ffb0")
        assignObjectWindow.row:getChildByIndex(5):setColor("#b0ffb0")
      elseif action == HOTKEY_ACTION.USE_CROSSHAIR then
        firstColumn:setText(tr("(use object on crosshair)"))
        firstColumn:setColor("#c87d7d")
        assignObjectWindow.row:getChildByIndex(3):setColor("#c87d7d")
        assignObjectWindow.row:getChildByIndex(5):setColor("#c87d7d")
      elseif action == HOTKEY_ACTION.USE_TARGET then
        firstColumn:setText(tr("(use object on target)"))
        firstColumn:setColor("#ffb0b0")
        assignObjectWindow.row:getChildByIndex(3):setColor("#ffb0b0")
        assignObjectWindow.row:getChildByIndex(5):setColor("#ffb0b0")
      elseif action == HOTKEY_ACTION.EQUIP then
        firstColumn:setText(tr("(equip/unequip object)"))
        firstColumn:setColor("#bfbf00")
        assignObjectWindow.row:getChildByIndex(3):setColor("#bfbf00")
        assignObjectWindow.row:getChildByIndex(5):setColor("#bfbf00")
      elseif action == HOTKEY_ACTION.EQUIP then
        firstColumn:setText(tr("(use object)"))
        firstColumn:setColor("#b0b0ff")
        assignObjectWindow.row:getChildByIndex(3):setColor("#b0b0ff")
        assignObjectWindow.row:getChildByIndex(5):setColor("#b0b0ff")
      end
    elseif assignObjectWindow.actionEdit == ActionEdit.ASSIGN then
      firstColumn:destroy()
      firstColumn = g_ui.createWidget("EditableHotkeysTableColumn")
      assignObjectWindow.row:insertChild(1, firstColumn)
      firstColumn:setWidth(286)

      updateHotkeyItem(firstColumn.item, data.itemId, data.itemSubType)
      if action == HOTKEY_ACTION.USE_YOURSELF then
        firstColumn:setText(tr("(use object on yourself)"))
        firstColumn:setColor("#b0ffb0")
        assignObjectWindow.row:getChildByIndex(3):setColor("#b0ffb0")
        assignObjectWindow.row:getChildByIndex(5):setColor("#b0ffb0")
      elseif action == HOTKEY_ACTION.USE_CROSSHAIR then
        firstColumn:setText(tr("(use object on crosshair)"))
        firstColumn:setColor("#c87d7d")
        assignObjectWindow.row:getChildByIndex(3):setColor("#c87d7d")
        assignObjectWindow.row:getChildByIndex(5):setColor("#c87d7d")
      elseif action == HOTKEY_ACTION.USE_TARGET then
        firstColumn:setText(tr("(use object on target)"))
        firstColumn:setColor("#ffb0b0")
        assignObjectWindow.row:getChildByIndex(3):setColor("#ffb0b0")
        assignObjectWindow.row:getChildByIndex(5):setColor("#ffb0b0")
      elseif action == HOTKEY_ACTION.EQUIP then
        firstColumn:setText(tr("(equip/unequip object)"))
        firstColumn:setColor("#bfbf00")
        assignObjectWindow.row:getChildByIndex(3):setColor("#bfbf00")
        assignObjectWindow.row:getChildByIndex(5):setColor("#bfbf00")
      elseif action == HOTKEY_ACTION.USE then
        firstColumn:setText(tr("(use object)"))
        firstColumn:setColor("#b0b0ff")
        assignObjectWindow.row:getChildByIndex(3):setColor("#b0b0ff")
        assignObjectWindow.row:getChildByIndex(5):setColor("#b0b0ff")
      end
      firstColumn.edit.onClick = editHotkeyAction
      firstColumn.edit:show()
    end
    local change = table.findbyfield(changedHotkeys, "hotkeyId", assignObjectWindow.row.hotkeyId)
    if change then
      change.action = action
      change.edit = true
      change.data = data
    else
      table.insert(changedHotkeys, { hotkeyId = assignObjectWindow.row.hotkeyId, data = data, action = action, edit = true })
    end
  else
    preAddHotkey(action, data)
  end

  if ok then
    assignObjectWindow:hide()
    optionsWindow:show()
  end
end

function selectObject(widget, mousePosition, mouseButton)
  if mouseButton == MouseLeftButton then
    local clickedWidget = modules.game_interface.getRootPanel():recursiveGetChildByPos(mousePosition, false)
    if clickedWidget and clickedWidget:getStyle().__class == "UIItem" then
      local item = clickedWidget:getItem()
      if item then
        local itemId = item:getId()
        local subType = item:getSubType()
        local thingType = g_things.getThingType(itemId, ThingCategoryItem)
        assignObjectWindow.panel.actions.yourself:setEnabled(thingType:isMultiUse())
        assignObjectWindow.panel.actions.crosshair:setEnabled(thingType:isMultiUse())
        assignObjectWindow.panel.actions.target:setEnabled(thingType:isMultiUse())
        assignObjectWindow.panel.actions.equip:setEnabled(thingType:isCloth())
        assignObjectWindow.panel.actions.use:setEnabled(not thingType:isMultiUse())

        if thingType:isMultiUse() then
          assignObjectGroup:selectWidget(assignObjectWindow.panel.actions.yourself)
        elseif thingType:isCloth() then
          assignObjectGroup:selectWidget(assignObjectWindow.panel.actions.equip)
        else
          assignObjectGroup:selectWidget(assignObjectWindow.panel.actions.use)
        end

        assignObjectWindow.panel.item:setItemId(itemId)
        assignObjectWindow.panel.item:setItemSubType(subType)
      end
    end
  end

  assignObjectWindow:show()
  assignObjectWindow:raise()
  assignObjectWindow:focus()

  g_mouse.popCursor('target')
  widget:ungrabMouse()
  modules.game_interface.getMapPanel():blockNextMouseRelease(true)

  return true
end

function openAssignObject(position, actionEdit, row)
  if actionEdit and actionEdit == ActionEdit.EDIT then
    assignObjectWindow.panel.item:setItemId(row.actionData.itemId)
    assignObjectWindow.panel.item:setItemSubType(row.actionData.itemSubType)

    local thingType = g_things.getThingType(row.actionData.itemId, ThingCategoryItem)
    assignObjectWindow.panel.actions.yourself:setEnabled(thingType:isMultiUse())
    assignObjectWindow.panel.actions.crosshair:setEnabled(thingType:isMultiUse())
    assignObjectWindow.panel.actions.target:setEnabled(thingType:isMultiUse())
    assignObjectWindow.panel.actions.equip:setEnabled(thingType:isCloth())
    assignObjectWindow.panel.actions.use:setEnabled(not thingType:isMultiUse())

    if thingType:isMultiUse() then
      assignObjectGroup:selectWidget(assignObjectWindow.panel.actions.yourself)
    elseif thingType:isCloth() then
      assignObjectGroup:selectWidget(assignObjectWindow.panel.actions.equip)
    else
      assignObjectGroup:selectWidget(assignObjectWindow.panel.actions.use)
    end

    assignObjectWindow:show()
    assignObjectWindow:raise()
    assignObjectWindow:focus()
  else
    assignObjectWindow.panel.item:setItem(nil)
    showMouseGrabber()
  end

  assignObjectWindow.actionEdit = actionEdit
  assignObjectWindow.row = row

  optionsWindow:hide()
end

function assignText(ok)
  local isAuto = assignTextWindow.auto:isChecked()
  local data = { isAuto = isAuto, text = assignTextWindow.field:getText():trim() }
  local action = isAuto and HOTKEY_ACTION.TEXT_AUTO or HOTKEY_ACTION.TEXT

  if assignTextWindow.actionEdit then
    assignTextWindow.row.actionData = data
    assignTextWindow.row.action = action
    local firstColumn = assignTextWindow.row:getChildByIndex(1)
    if assignTextWindow.actionEdit == ActionEdit.EDIT then
      firstColumn:setText(data.text)
      firstColumn:setColor(data.isAuto and "#ffffff" or "#c0c0c0")
      assignTextWindow.row:getChildByIndex(3):setColor(data.isAuto and "#ffffff" or "#c0c0c0")
      assignTextWindow.row:getChildByIndex(5):setColor(data.isAuto and "#ffffff" or "#c0c0c0")
    elseif assignTextWindow.actionEdit == ActionEdit.ASSIGN then
      firstColumn:destroy()
      firstColumn = g_ui.createWidget("EditableKeybindsTableColumn")
      assignTextWindow.row:insertChild(1, firstColumn)
      firstColumn:setWidth(286)
      firstColumn:setText(data.text)
      firstColumn:setColor(data.isAuto and "#ffffff" or "#c0c0c0")
      firstColumn.edit.onClick = editHotkeyAction
      firstColumn.edit:show()
      assignTextWindow.row:getChildByIndex(3):setColor(data.isAuto and "#ffffff" or "#c0c0c0")
      assignTextWindow.row:getChildByIndex(5):setColor(data.isAuto and "#ffffff" or "#c0c0c0")
    end
    local change = table.findbyfield(changedHotkeys, "hotkeyId", assignTextWindow.row.hotkeyId)
    if change then
      change.action = action
      change.edit = true
      change.data = data
    else
      table.insert(changedHotkeys, { hotkeyId = assignTextWindow.row.hotkeyId, data = data, action = action, edit = true })
    end
  else
    preAddHotkey(action, data)
  end

  if ok then
    assignTextWindow.field:clearText()
    assignTextWindow:hide()
    optionsWindow:show()
  end
end

function openAssignText(position, actionEdit, row)
  if actionEdit and actionEdit == ActionEdit.EDIT then
    assignTextWindow.field:setText(row.actionData.text)
    assignTextWindow.field:setCursorPos(9999)
  end

  assignTextWindow.actionEdit = actionEdit
  assignTextWindow.row = row

  assignTextWindow:show()
  assignTextWindow:raise()
  assignTextWindow:focus()

  optionsWindow:hide()
end

function cancelActionAssignment()
  assignSpellWindow:hide()

  assignObjectWindow:hide()

  assignTextWindow.field:clearText()
  assignTextWindow:hide()

  optionsWindow:show()
end

function preAddHotkey(action, data)
  local preset = keybindsPanel.presets.list:getCurrentOption().text
  local chatMode = getChatMode()
  local hotkeyId = #changedHotkeys + 1

  if Keybind.hotkeys[chatMode] and Keybind.hotkeys[chatMode][preset] then
    hotkeyId = hotkeyId + #Keybind.hotkeys[chatMode][preset]
  end

  table.insert(changedHotkeys, { hotkeyId = hotkeyId, action = action, data = data, new = true })

  addHotkey(hotkeyId, action, data)
end

function addKeybind(category, action, primary, secondary)
  local rawText = string.format("%s: %s", category, action)
  local text = string.format("[color=#ffffff]%s:[/color] %s", category, action)
  local tooltip = nil

  if rawText:len() > actionNameLimit then
    tooltip = rawText
    -- 15 and 8 are length of color codes
    text = text:sub(1, actionNameLimit + 15 + 8) .. "..."
  end

  local row = keybindsPanel.tablePanel.keybinds:addRow({
    {
      coloredText = { text = text, color = "#c0c0c0" },
      width = 286
    },
    { style = "VerticalSeparator" },
    {
      style = "EditableKeybindsTableColumn",
      text = primary,
      width = 100
    },
    { style = "VerticalSeparator" },
    {
      style = "EditableKeybindsTableColumn",
      text = secondary,
      width = 100
    },
  })

  row.category = category
  row.action = action

  if tooltip then
    row:setTooltip(tooltip)
  end

  row:getChildByIndex(3).edit.onClick = editKeybindPrimary
  row:getChildByIndex(5).edit.onClick = editKeybindSecondary
end

function addHotkey(hotkeyId, action, data, primary, secondary)
  local row = nil

  if action == HOTKEY_ACTION.USE_YOURSELF then
    row = keybindsPanel.tablePanel.keybinds:addRow({
      {
        id = "actionColumn",
        style = "EditableHotkeysTableColumn",
        text = tr("(use object on yourself)"),
        color = "#b0ffb0",
        width = 286
      },
      { style = "VerticalSeparator" },
      {
        style = "EditableKeybindsTableColumn",
        text = primary,
        color = "#b0ffb0",
        width = 100
      },
      { style = "VerticalSeparator" },
      {
        style = "EditableKeybindsTableColumn",
        text = secondary,
        color = "#b0ffb0",
        width = 100
      },
    })

    updateHotkeyItem(row.actionColumn.item, data.itemId, data.itemSubType)
  elseif action == HOTKEY_ACTION.USE_CROSSHAIR then
    row = keybindsPanel.tablePanel.keybinds:addRow({
      {
        id = "actionColumn",
        style = "EditableHotkeysTableColumn",
        text = tr("(use object on crosshair)"),
        color = "#c87d7d",
        width = 286
      },
      { style = "VerticalSeparator" },
      {
        style = "EditableKeybindsTableColumn",
        text = primary,
        color = "#c87d7d",
        width = 100
      },
      { style = "VerticalSeparator" },
      {
        style = "EditableKeybindsTableColumn",
        text = secondary,
        color = "#c87d7d",
        width = 100
      },
    })

    updateHotkeyItem(row.actionColumn.item, data.itemId, data.itemSubType)
  elseif action == HOTKEY_ACTION.USE_TARGET then
    row = keybindsPanel.tablePanel.keybinds:addRow({
      {
        id = "actionColumn",
        style = "EditableHotkeysTableColumn",
        text = tr("(use object on target)"),
        color = "#ffb0b0",
        width = 286
      },
      { style = "VerticalSeparator" },
      {
        style = "EditableKeybindsTableColumn",
        text = primary,
        color = "#ffb0b0",
        width = 100
      },
      { style = "VerticalSeparator" },
      {
        style = "EditableKeybindsTableColumn",
        text = secondary,
        color = "#ffb0b0",
        width = 100
      },
    })

    updateHotkeyItem(row.actionColumn.item, data.itemId, data.itemSubType)
  elseif action == HOTKEY_ACTION.EQUIP then
    row = keybindsPanel.tablePanel.keybinds:addRow({
      {
        id = "actionColumn",
        style = "EditableHotkeysTableColumn",
        text = tr("(equip/unequip object)"),
        color = "#bfbf00",
        width = 286
      },
      { style = "VerticalSeparator" },
      {
        style = "EditableKeybindsTableColumn",
        text = primary,
        color = "#bfbf00",
        width = 100
      },
      { style = "VerticalSeparator" },
      {
        style = "EditableKeybindsTableColumn",
        text = secondary,
        color = "#bfbf00",
        width = 100
      },
    })

    updateHotkeyItem(row.actionColumn.item, data.itemId, data.itemSubType)
  elseif action == HOTKEY_ACTION.USE then
    row = keybindsPanel.tablePanel.keybinds:addRow({
      {
        id = "actionColumn",
        style = "EditableHotkeysTableColumn",
        text = tr("(use object)"),
        color = "#b0b0ff",
        width = 286
      },
      { style = "VerticalSeparator" },
      {
        style = "EditableKeybindsTableColumn",
        text = primary,
        color = "#b0b0ff",
        width = 100
      },
      { style = "VerticalSeparator" },
      {
        style = "EditableKeybindsTableColumn",
        text = secondary,
        color = "#b0b0ff",
        width = 100
      },
    })

    updateHotkeyItem(row.actionColumn.item, data.itemId, data.itemSubType)
  elseif action == HOTKEY_ACTION.TEXT then
    local text = data.text
    local tooltip = nil

    if text:len() > actionNameLimit then
      tooltip = text
      text = text:sub(1, actionNameLimit) .. "..."
    end
    row = keybindsPanel.tablePanel.keybinds:addRow({
      {
        style = "EditableKeybindsTableColumn",
        text = text,
        color = "#c0c0c0",
        width = 286
      },
      { style = "VerticalSeparator" },
      {
        style = "EditableKeybindsTableColumn",
        text = primary,
        color = "#c0c0c0",
        width = 100
      },
      { style = "VerticalSeparator" },
      {
        style = "EditableKeybindsTableColumn",
        text = secondary,
        color = "#c0c0c0",
        width = 100
      },
    })

    if tooltip then
      row:setTooltip(tooltip)
    end
  elseif action == HOTKEY_ACTION.TEXT_AUTO then
    local text = data.text
    local tooltip = nil

    if text:len() > actionNameLimit then
      tooltip = text
      text = text:sub(1, actionNameLimit) .. "..."
    end

    row = keybindsPanel.tablePanel.keybinds:addRow({
      {
        style = "EditableKeybindsTableColumn",
        text = text,
        color = "#ffffff",
        width = 286
      },
      { style = "VerticalSeparator" },
      {
        style = "EditableKeybindsTableColumn",
        text = primary,
        color = "#ffffff",
        width = 100
      },
      { style = "VerticalSeparator" },
      {
        style = "EditableKeybindsTableColumn",
        text = secondary,
        color = "#ffffff",
        width = 100
      },
    })

    if tooltip then
      row:setTooltip(tooltip)
    end
  elseif action == HOTKEY_ACTION.SPELL then
    local text = data.words
    if data.parameter then
      text = text .. " " .. data.parameter
    end

    local tooltip = nil

    if text:len() > actionNameLimit then
      tooltip = text
      text = text:sub(1, actionNameLimit) .. "..."
    end

    row = keybindsPanel.tablePanel.keybinds:addRow({
      {
        style = "EditableKeybindsTableColumn",
        text = text,
        color = "#ffffff",
        width = 286
      },
      { style = "VerticalSeparator" },
      {
        style = "EditableKeybindsTableColumn",
        text = primary,
        color = "#ffffff",
        width = 100
      },
      { style = "VerticalSeparator" },
      {
        style = "EditableKeybindsTableColumn",
        text = secondary,
        color = "#ffffff",
        width = 100
      },
    })

    if tooltip then
      row:setTooltip(tooltip)
    end
  end

  if row then
    row:setId("hotkey" .. hotkeyId)
    row.hotkeyId = hotkeyId
    row.action = action
    row.actionData = data
    row:getChildByIndex(1).edit.onClick = editHotkeyAction
    row:getChildByIndex(3).edit.onClick = editHotkeyPrimary
    row:getChildByIndex(5).edit.onClick = editHotkeySecondary
  end
end

function updateHotkeyItem(itemWidget, itemId, subType)
  if not itemId or itemId == 0 then
    return
  end

  local thingType = g_things.getThingType(itemId, ThingCategoryItem)
  local exactSize = thingType:getExactSize()
  local spriteSize = g_sprites.spriteSize()
  local sizeDiff = spriteSize - exactSize
  local widgetSize = itemWidget:getWidth()
  local size = sizeDiff + widgetSize
  itemWidget:setMarginLeft((-sizeDiff / 2) + 2)
  itemWidget:setSize({ width = size, height = size })
  itemWidget:setItemId(itemId)
  itemWidget:setItemSubType(subType)
end

function clearHotkey(row)
  table.insert(changedHotkeys, { hotkeyId = row.hotkeyId, remove = true })
  keybindsPanel.tablePanel.keybinds:removeRow(row)
end

function editHotkeyAction(button)
  local column = button:getParent()
  local row = column:getParent()

  local mousePos = g_window.getMousePosition()
  local menu = g_ui.createWidget("PopupMenu")
  menu:setGameMenu(true)

  if row.action == HOTKEY_ACTION.SPELL then
    menu:addOption(tr("Edit Spell"), function() openAssignSpell(nil, ActionEdit.EDIT, row) end)
  else
    menu:addOption(tr("Assign Spell"), function() openAssignSpell(nil, ActionEdit.ASSIGN, row) end)
  end

  if row.action ~= HOTKEY_ACTION.SPELL and row.action ~= HOTKEY_ACTION.TEXT and row.action ~= HOTKEY_ACTION.TEXT_AUTO then
    menu:addOption(tr("Edit Object"), function() openAssignObject(nil, ActionEdit.EDIT, row) end)
  else
    menu:addOption(tr("Assign Object"), function() openAssignObject(nil, ActionEdit.ASSIGN, row) end)
  end

  if row.action == HOTKEY_ACTION.TEXT or row.action == HOTKEY_ACTION.TEXT_AUTO then
    menu:addOption(tr("Edit Text"), function() openAssignText(nil, ActionEdit.EDIT, row) end)
  else
    menu:addOption(tr("Assign Text"), function() openAssignText(nil, ActionEdit.ASSIGN, row) end)
  end

  menu:addSeparator()
  menu:addOption(tr("Clear Action"), function() clearHotkey(row) end)

  menu:display(mousePos)
end

function editHotkeyKey(text)
  keyEditWindow.buttons.cancel.onClick = function()
    disconnect(keyEditWindow, { onKeyDown = editKeybindKeyDown })
    keyEditWindow:hide()
    keyEditWindow:ungrabKeyboard()
    show()
  end

  keyEditWindow.info:setText(tr("Click \"Ok\" to assign the keybind. Click \"Clear\" to remove the keybind from \"%s\".", text))
  keyEditWindow.alone:setVisible(false)

  connect(keyEditWindow, { onKeyDown = editKeybindKeyDown })

  keyEditWindow:show()
  keyEditWindow:raise()
  keyEditWindow:focus()
  keyEditWindow:grabKeyboard()
  hide()
end

function editHotkeyPrimary(button)
  local column = button:getParent()
  local row = column:getParent()
  local text = row:getChildByIndex(1):getText()
  local hotkeyId = row.hotkeyId
  local preset = keybindsPanel.presets.list:getCurrentOption().text

  keyEditWindow:setText(tr("Edit Primary Key for \"%s\"", text))
  keyEditWindow.keyCombo:setText(Keybind.getHotkeyKeys(hotkeyId, preset, getChatMode()).primary)

  editHotkeyKey(text)

  keyEditWindow.buttons.ok.onClick = function()
    local keyCombo = keyEditWindow.keyCombo:getText()

    column:setText(keyEditWindow.keyCombo:getText())

    local changed = table.findbyfield(changedHotkeys, "hotkeyId", hotkeyId)
    if changed then
      changed.primary = keyCombo
      if not changed.secondary then
        changed.secondary = Keybind.getHotkeyKeys(hotkeyId, preset, getChatMode()).secondary
      end
      changed.editKey = true
    else
      table.insert(changedHotkeys, { hotkeyId = hotkeyId, primary = keyCombo, secondary = Keybind.getHotkeyKeys(hotkeyId, preset, getChatMode()).secondary, editKey = true })
    end

    disconnect(keyEditWindow, { onKeyDown = editKeybindKeyDown })
    keyEditWindow:hide()
    keyEditWindow:ungrabKeyboard()
    show()
  end

  keyEditWindow.buttons.clear.onClick = function()
    column:setText("")

    local changed = table.findbyfield(changedHotkeys, "hotkeyId", hotkeyId)
    if changed then
      changed.primary = nil
      if not changed.secondary then
        changed.secondary = Keybind.getHotkeyKeys(hotkeyId, preset, getChatMode()).secondary
      end
      changed.editKey = true
    else
      table.insert(changedHotkeys, { hotkeyId = hotkeyId, secondary = Keybind.getHotkeyKeys(hotkeyId, preset, getChatMode()).secondary, editKey = true })
    end

    disconnect(keyEditWindow, { onKeyDown = editKeybindKeyDown })
    keyEditWindow:hide()
    keyEditWindow:ungrabKeyboard()
    show()
  end
end

function editHotkeySecondary(button)
  local column = button:getParent()
  local row = column:getParent()
  local text = row:getChildByIndex(1):getText()
  local hotkeyId = row.hotkeyId
  local preset = keybindsPanel.presets.list:getCurrentOption().text

  keyEditWindow:setText(tr("Edit Secondary Key for \"%s\"", text))
  keyEditWindow.keyCombo:setText(Keybind.getHotkeyKeys(hotkeyId, preset, getChatMode()).secondary)

  editHotkeyKey(text)

  keyEditWindow.buttons.ok.onClick = function()
    local keyCombo = keyEditWindow.keyCombo:getText()

    column:setText(keyEditWindow.keyCombo:getText())

    if changedHotkeys[hotkeyId] then
      if not changedHotkeys[hotkeyId].primary then
        changedHotkeys[hotkeyId].primary = Keybind.getHotkeyKeys(hotkeyId, preset, getChatMode()).primary
      end
      changedHotkeys[hotkeyId].secondary = keyCombo
      changedHotkeys[hotkeyId].editKey = true
    else
      table.insert(changedHotkeys, { hotkeyId = hotkeyId, primary = Keybind.getHotkeyKeys(hotkeyId, preset, getChatMode()).primary, secondary = keyCombo, editKey = true })
    end

    disconnect(keyEditWindow, { onKeyDown = editKeybindKeyDown })
    keyEditWindow:hide()
    keyEditWindow:ungrabKeyboard()
    show()
  end

  keyEditWindow.buttons.clear.onClick = function()
    column:setText("")

    if changedHotkeys[hotkeyId] then
      if not changedHotkeys[hotkeyId].primary then
        changedHotkeys[hotkeyId].primary = Keybind.getHotkeyKeys(hotkeyId, preset, getChatMode()).primary
      end
      changedHotkeys[hotkeyId].secondary = nil
      changedHotkeys[hotkeyId].editKey = true
    else
      table.insert(changedHotkeys, { hotkeyId = hotkeyId, primary = Keybind.getHotkeyKeys(hotkeyId, preset, getChatMode()).primary, editKey = true })
    end

    disconnect(keyEditWindow, { onKeyDown = editKeybindKeyDown })
    keyEditWindow:hide()
    keyEditWindow:ungrabKeyboard()
    show()
  end
end

function searchActions(field, text, oldText)
  if actionSearchEvent then
    removeEvent(actionSearchEvent)
  end

  actionSearchEvent = scheduleEvent(performeSearchActions, 200)
end

function performeSearchActions()
  local searchText = keybindsPanel.search.field:getText():trim():lower()

  local rows = keybindsPanel.tablePanel.keybinds.dataSpace:getChildren()
  if searchText:len() > 0 then
    for _, row in ipairs(rows) do
      row:hide()
    end

    for _, row in ipairs(rows) do
      local actionText = row:getChildByIndex(1):getText():lower()
      local primaryText = row:getChildByIndex(3):getText():lower()
      local secondaryText = row:getChildByIndex(5):getText():lower()
      if actionText:find(searchText) or primaryText:find(searchText) or secondaryText:find(searchText) then
        row:show()
      end
    end
  else
    for _, row in ipairs(rows) do
      row:show()
    end
  end

  removeEvent(actionSearchEvent)
  actionSearchEvent = nil
end

function chatModeChange()
  changedHotkeys = {}
  changedKeybinds = {}

  keybindsPanel.search.field:clearText()

  if optionsTabBar.currentTab == keybindsButton then
    updateKeybinds()
  elseif optionsTabBar.currentTab == hotkeysButton then
    updateHotkeys()
  end
end

function getChatMode()
  if chatModeGroup:getSelectedWidget() == keybindsPanel.chatMode.on then
    return CHAT_MODE.ON
  end

  return CHAT_MODE.OFF
end

function filterAssignSpell()
  local availableOnly = assignSpellWindow.availableSpellsOnly:isChecked()
  local searchText = assignSpellWindow.search.field:getText():trim():lower()
  local filterByText = searchText:len() > 0

  local player = g_game.getLocalPlayer()
  local isPremium = false
  local level = 0
  local vocation = 0
  local focusedSpell = assignSpellWindow.spells.list:getFocusedChild()
  local spells = assignSpellWindow.spells.list:getChildren()

  if player then
    isPremium = player:isPremium()
    level = player:getLevel()
    vocation = VocationClientToServer[player:getVocation()]
  end

  for _, spellWidget in ipairs(spells) do
    local spell = SpellInfo["Default"][spellWidget.spellName]
    local enabled = true
    local filtered = false

    if spell.premium then
      enabled = isPremium
    end

    if enabled then
      enabled = spell.level <= level
    end

    if enabled then
      if table.contains(spell.vocations, vocation) then
        enabled = true
      end
    end

    if filterByText then
      filtered = spellWidget:getText():lower():find(searchText)
    end

    if availableOnly then
      if filterByText then
        spellWidget:setVisible(filtered and enabled)
      else
        spellWidget:setVisible(enabled)
      end
      spellWidget:setOn(true)
    else
      if filterByText then
        spellWidget:setVisible(filtered)
      else
        spellWidget:setVisible(true)
      end
      spellWidget:setOn(enabled)
    end
  end

  if focusedSpell:isVisible() then
    assignSpellWindow.spells.list:ensureChildVisible(focusedSpell)
  else
    assignSpellWindow.spells.list:focusNextChild(MouseFocusReason, true)
  end
end
