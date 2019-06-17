local defaultOptions = {
  showFps = true,
  showPing = true,
  fullscreen = false,
  classicView = true,
  classicControl = true,
  smartWalk = false,
  extentedPreWalking = true,
  autoChaseOverride = true,
  showStatusMessagesInConsole = true,
  showEventMessagesInConsole = true,
  showInfoMessagesInConsole = true,
  showTimestampsInConsole = true,
  showLevelsInConsole = true,
  showPrivateMessagesInConsole = true,
  showPrivateMessagesOnScreen = true,
  panels = 6,
  openContainersInThirdPanel = true,
  backgroundFrameRate = 100,
  enableAudio = false,
  enableMusicSound = false,
  musicSoundVolume = 100,
  enableLights = false,
  floorFading = 500,
  crosshair = 1,
  ambientLight = 100,
  optimizationLevel = 1,
  displayNames = true,
  displayHealth = true,
  displayMana = true,
  showHealthManaCircle = true,
  hidePlayerBars = true,
  highlightThingsUnderCursor = true,
  topHealtManaBar = true,
  displayText = true,
  dontStretchShrink = false,
  turnDelay = 50,
  hotkeyDelay = 50,
}

local optionsWindow
local optionsButton
local optionsTabBar
local options = {}
local extraOptions = {}
local generalPanel
local interfacePanel
local consolePanel
local graphicsPanel
local soundPanel
local extrasPanel
local audioButton

function init()
  for k,v in pairs(defaultOptions) do
    g_settings.setDefault(k, v)
    options[k] = v
  end
  for _, v in ipairs(g_extras.getAll()) do
	  extraOptions[v] = g_extras.get(v)
    g_settings.setDefault("extras_" .. v, extraOptions[v])
  end

  optionsWindow = g_ui.displayUI('options')
  optionsWindow:hide()

  optionsTabBar = optionsWindow:getChildById('optionsTabBar')
  optionsTabBar:setContentWidget(optionsWindow:getChildById('optionsTabContent'))

  g_keyboard.bindKeyDown('Ctrl+Shift+F', function() toggleOption('fullscreen') end)
  g_keyboard.bindKeyDown('Ctrl+N', toggleDisplays)

  generalPanel = g_ui.loadUI('game')
  optionsTabBar:addTab(tr('Game'), generalPanel, '/images/optionstab/game')
  
  interfacePanel = g_ui.loadUI('interface')
  optionsTabBar:addTab(tr('Interface'), interfacePanel, '/images/optionstab/game')  

  consolePanel = g_ui.loadUI('console')
  optionsTabBar:addTab(tr('Console'), consolePanel, '/images/optionstab/console')

  graphicsPanel = g_ui.loadUI('graphics')
  optionsTabBar:addTab(tr('Graphics'), graphicsPanel, '/images/optionstab/graphics')

  audioPanel = g_ui.loadUI('audio')
  optionsTabBar:addTab(tr('Audio'), audioPanel, '/images/optionstab/audio')

  extrasPanel = g_ui.createWidget('Panel')
  for _, v in ipairs(g_extras.getAll()) do
    local extrasButton = g_ui.createWidget('OptionCheckBox')
    extrasButton:setId(v)
    extrasButton:setText(g_extras.getDescription(v))
    extrasPanel:addChild(extrasButton)
  end
  
  optionsTabBar:addTab(tr('Extras'), extrasPanel, '/images/optionstab/extras')

  optionsButton = modules.client_topmenu.addLeftButton('optionsButton', tr('Options'), '/images/topbuttons/options', toggle)
  audioButton = modules.client_topmenu.addLeftButton('audioButton', tr('Audio'), '/images/topbuttons/audio', function() toggleOption('enableAudio') end)

  addEvent(function() setup() end)
end

function terminate()
  g_keyboard.unbindKeyDown('Ctrl+Shift+F')
  g_keyboard.unbindKeyDown('Ctrl+N')
  optionsWindow:destroy()
  optionsButton:destroy()
  audioButton:destroy()
end

function setup()
  -- load options
  for k,v in pairs(defaultOptions) do
    if type(v) == 'boolean' then
      setOption(k, g_settings.getBoolean(k), true)
    elseif type(v) == 'number' then
      setOption(k, g_settings.getNumber(k), true)
    end
  end
  
  for _, v in ipairs(g_extras.getAll()) do
	g_extras.set(v, g_settings.getBoolean("extras_" .. v))
	local widget = extrasPanel:recursiveGetChildById(v)
	if widget then
        widget:setChecked(g_extras.get(v))
    end
  end
  
end

function toggle()
  if optionsWindow:isVisible() then
    hide()
  else
    show()
  end
end

function show()
  optionsWindow:show()
  optionsWindow:raise()
  optionsWindow:focus()
end

function hide()
  optionsWindow:hide()
end

function toggleDisplays()
  if options['displayNames'] and options['displayHealth'] and options['displayMana'] then
    setOption('displayNames', false)
  elseif options['displayHealth'] then
    setOption('displayHealth', false)
    setOption('displayMana', false)
  else
    if not options['displayNames'] and not options['displayHealth'] then
      setOption('displayNames', true)
    else
      setOption('displayHealth', true)
      setOption('displayMana', true)
    end
  end
end

function toggleOption(key) 
  setOption(key, not getOption(key))
end

function setOption(key, value, force)
  if extraOptions[key] ~= nil then
    g_extras.set(key, value)
    g_settings.set("extras_" .. key, value)
    return
  end
   
  if not force and options[key] == value then return end
  local gameMapPanel = modules.game_interface.getMapPanel()

  if key == 'showFps' then
    modules.client_topmenu.setFpsVisible(value)
  elseif key == 'showPing' then
    modules.client_topmenu.setPingVisible(value)
  elseif key == 'fullscreen' then
    g_window.setFullscreen(value)
  elseif key == 'enableAudio' then
    if g_sounds ~= nil then
      g_sounds.setAudioEnabled(value)
    end
    if value then
      audioButton:setIcon('/images/topbuttons/audio')
    else
      audioButton:setIcon('/images/topbuttons/audio_mute')
    end
  elseif key == 'enableMusicSound' then
    if g_sounds ~= nil then
      g_sounds.getChannel(SoundChannels.Music):setEnabled(value)
    end
  elseif key == 'musicSoundVolume' then
    if g_sounds ~= nil then
      g_sounds.getChannel(SoundChannels.Music):setGain(value/100)
    end
    audioPanel:getChildById('musicSoundVolumeLabel'):setText(tr('Music volume: %d', value))
  elseif key == 'showHealthManaCircle' then
    modules.game_interface.healthCircle:setVisible(value)
    modules.game_interface.healthCircleFront:setVisible(value)
    modules.game_interface.manaCircle:setVisible(value)
    modules.game_interface.manaCircleFront:setVisible(value)
  elseif key == 'classicView' and not force then
    local viewMode = 1
    if value then
      viewMode = 0
    end    
    modules.game_interface.setupViewMode(viewMode)    
  elseif key == 'backgroundFrameRate' then
    local text, v = value, value
    if value <= 0 or value >= 201 then text = 'max' v = 0 end
    graphicsPanel:getChildById('backgroundFrameRateLabel'):setText(tr('Game framerate limit: %s', text))
    g_app.setMaxFps(v)
  elseif key == 'enableLights' then
    gameMapPanel:setDrawLights(value and options['ambientLight'] < 100)
    graphicsPanel:getChildById('ambientLight'):setEnabled(value)
    graphicsPanel:getChildById('ambientLightLabel'):setEnabled(value)
  elseif key == 'floorFading' then
    gameMapPanel:setFloorFading(value)
    interfacePanel:getChildById('floorFadingLabel'):setText(tr('Floor fading: %s ms', value))
  elseif key == 'crosshair' then
    if value == 1 then
      gameMapPanel:setCrosshair("")    
    elseif value == 2 then
      gameMapPanel:setCrosshair("/data/images/crosshair/default.png")        
    elseif value == 3 then
      gameMapPanel:setCrosshair("/data/images/crosshair/full.png")    
    end
  elseif key == 'ambientLight' then
    graphicsPanel:getChildById('ambientLightLabel'):setText(tr('Ambient light: %s%%', value))
    gameMapPanel:setMinimumAmbientLight(value/100)
    gameMapPanel:setDrawLights(options['enableLights'] and value < 100)
  elseif key == 'optimizationLevel' then
    g_adaptiveRenderer.setLevel(value - 2)
  elseif key == 'displayNames' then
    gameMapPanel:setDrawNames(value)
  elseif key == 'displayHealth' then
    gameMapPanel:setDrawHealthBars(value)
  elseif key == 'displayMana' then
    gameMapPanel:setDrawManaBar(value)
  elseif key == 'hidePlayerBars' then
    gameMapPanel:setDrawPlayerBars(not value)
  elseif key == 'topHealtManaBar' then
    modules.game_interface.healthBar:setVisible(value)
    modules.game_interface.manaBar:setVisible(value)
  elseif key == 'displayText' then
    gameMapPanel:setDrawTexts(value)
  elseif key == 'dontStretchShrink' then
    addEvent(function()
      modules.game_interface.updateStretchShrink()
    end)
  elseif key == 'turnDelay' then
    generalPanel:getChildById('turnDelayLabel'):setText(tr('Turn delay: %sms', value))
  elseif key == 'hotkeyDelay' then
    generalPanel:getChildById('hotkeyDelayLabel'):setText(tr('Hotkey delay: %sms', value))
  end
  if key == 'newWalking' then
	g_app.setNewWalking(value)
  elseif key == 'newAutoWalking' then
    g_app.setNewAutoWalking(value)
  elseif key == 'newRendering' then
    g_app.setNewRendering(value)
  elseif key == 'newTextRendering' then
    g_app.setNewTextRendering(value)
  elseif key == 'newBotDetection' then
    g_app.setNewBotDetection(value)
  elseif key == 'newQTMLCache' then
    g_app.setNewQTMLCache(value)
  elseif key == 'newBattleList' then
    g_app.setNewBattleList(value)
  end
  

  -- change value for keybind updates
  for _,panel in pairs(optionsTabBar:getTabsPanel()) do
    local widget = panel:recursiveGetChildById(key)
    if widget then
      if widget:getStyle().__class == 'UICheckBox' then
        widget:setChecked(value)
      elseif widget:getStyle().__class == 'UIScrollBar' then
        widget:setValue(value)
      elseif widget:getStyle().__class == 'UIComboBox' then
        if valur ~= nil or value < 1 then 
          value = 1
        end
        if widget.currentIndex ~= value then
          widget:setCurrentIndex(value)
        end
      end      
      break
    end
  end
  
  g_settings.set(key, value)
  options[key] = value
  
  if key == 'panels' and modules.game_interface ~= nil then
    modules.game_interface.refreshViewMode()
  end
end

function getOption(key)
  return options[key]
end

function addTab(name, panel, icon)
  optionsTabBar:addTab(name, panel, icon)
end

function addButton(name, func, icon)
  optionsTabBar:addButton(name, func, icon)
end
