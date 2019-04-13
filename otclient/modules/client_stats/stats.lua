UUID = nil
local statsWindow = nil
local statsButton = nil
local luaStats = nil
local luaCallback = nil
local mainStats = nil
local dispatcherStats = nil
local render = nil
local adaptiveRender = nil
local slowMain = nil

local updateEvent = nil
local iter = 0
local lastSend = 0
local sendInterval = 600 -- 10 m

function initUUID()
  UUID = g_settings.getString('report-uuid')
  if not UUID or #UUID ~= 36 then
    UUID = g_crypt.genUUID()
    g_settings.set('report-uuid', UUID)
  end
end

function init()
  statsButton = modules.client_topmenu.addLeftButton('statsButton', 'Debug Info', '/images/topbuttons/debug', toggle)
  statsButton:setOn(false)

  statsWindow = g_ui.displayUI('stats')
  statsWindow:hide()
    
  g_keyboard.bindKeyDown('Ctrl+D', toggle)

  luaStats = statsWindow:recursiveGetChildById('luaStats')
  luaCallback = statsWindow:recursiveGetChildById('luaCallback')
  mainStats = statsWindow:recursiveGetChildById('mainStats')
  dispatcherStats = statsWindow:recursiveGetChildById('dispatcherStats')
  render = statsWindow:recursiveGetChildById('render')
  adaptiveRender = statsWindow:recursiveGetChildById('adaptiveRender')
  slowMain = statsWindow:recursiveGetChildById('slowMain')
  
  lastSend = os.time()
  initUUID()
  
  updateEvent = scheduleEvent(update, 2000)
end

function terminate()
  statsWindow:destroy()
  statsButton:destroy()
  g_keyboard.unbindKeyDown('Ctrl+D')
  
  if updateEvent ~= nil then
	  removeEvent(updateEvent)
	  updateEvent = nil
  end
end

function onMiniWindowClose()
  statsButton:setOn(false)
end

function toggle()
  if statsButton:isOn() then
    statsWindow:hide()
    statsButton:setOn(false)
  else
    statsWindow:show()
    statsButton:setOn(true)
  end
end

function sendStats()
  if Services.stats == nil or Services.stats:len() < 6 then
    return
  end
  lastSend = os.time()
  local localPlayer = g_game.getLocalPlayer()
  local playerData = nil
  if localPlayer ~= nil then
    playerData = {
      name = localPlayer:getName(),
      position = localPlayer:getPosition()
    }
  end
  local data = {
    uid = UUID,
    stats = {},
    slow = {},
    render = g_adaptiveRenderer.getDebugInfo(),
    player = playerData,

    details = {
      report_delay = sendInterval,
      os = g_app.getOs(),
      graphics_vendor = g_graphics.getVendor(),
      graphics_renderer = g_graphics.getRenderer(),
      graphics_version = g_graphics.getVersion(),
      painter_engine = g_graphics.getPainterEngine(),
      fps = g_app.getFps(),
      fullscreen = tostring(g_window.isFullscreen()),
      window_width = g_window.getWidth(),
      window_height = g_window.getHeight(),
      player_name = g_game.getCharacterName(),
      world_name = g_game.getWorldName(),
      otserv_host = G.host,
      otserv_port = G.port,
      otserv_protocol = g_game.getProtocolVersion(),
      otserv_client = g_game.getClientVersion(),
      build_version = g_app.getVersion(),
      build_revision = g_app.getBuildRevision(),
      build_commit = g_app.getBuildCommit(),
      build_date = g_app.getBuildDate(),
      display_width = g_window.getDisplayWidth(),
      display_height = g_window.getDisplayHeight(),
      cpu = g_platform.getCPUName(),
      mem = g_platform.getTotalSystemMemory(),
      os_name = g_platform.getOSName()    
    }
  } 
  for i = 1, g_stats.types() do
    table.insert(data.stats, g_stats.get(i - 1, 10, false))
    table.insert(data.slow, g_stats.getSlow(i - 1, 50, 10, false))
    g_stats.clear(i - 1)
    g_stats.clearSlow(i - 1)
  end
  data = json.encode(data)
  g_http.post(Services.stats, data)
end

function update()
  updateEvent = scheduleEvent(update, 200)
  if lastSend + sendInterval < os.time() then
    sendStats()
  end
  
  if not statsWindow:isVisible() then
    return
  end
  
  local adaptive = "Adaptive: " .. g_adaptiveRenderer.getLevel() .. " | " .. g_adaptiveRenderer.getDebugInfo()
  render:setText(g_stats.get(2, 10, true))  
  adaptiveRender:setText(adaptive)
  mainStats:setText(g_stats.get(1, 5, true))
  dispatcherStats:setText(g_stats.get(3, 5, true))
  luaStats:setText(g_stats.get(4, 5, true))
  luaCallback:setText(g_stats.get(5, 5, true))
  slowMain:setText(g_stats.getSlow(3, 10, 10, true) .. "\n\n\n" .. g_stats.getSlow(1, 20, 20, true))  
end

