debugWindow = nil
debugButton = nil
local luaStats = nil
local luaCallback = nil
local mainStats = nil
local dispatcherStats = nil
local render = nil
local adaptiveRender = nil
local slowMain = nil

local updateEvent = nil
local iter = 0

function init()
  debugButton = modules.client_topmenu.addLeftButton('debugButton',
    'Debug Info', '/images/topbuttons/debug', toggle)
  debugButton:setOn(false)

  debugWindow = g_ui.loadUI('debug', modules.game_interface.getRightPanel())
    
  g_keyboard.bindKeyDown('Ctrl+D', toggle)

  luaStats = debugWindow:recursiveGetChildById('luaStats')
  luaCallback = debugWindow:recursiveGetChildById('luaCallback')
  mainStats = debugWindow:recursiveGetChildById('mainStats')
  dispatcherStats = debugWindow:recursiveGetChildById('dispatcherStats')
  render = debugWindow:recursiveGetChildById('render')
  adaptiveRender = debugWindow:recursiveGetChildById('adaptiveRender')
  slowMain = debugWindow:recursiveGetChildById('slowMain')
  
  updateEvent = scheduleEvent(update, 200)
  debugWindow:setup()
end

function terminate()
  debugWindow:destroy()
  debugButton:destroy()
  g_keyboard.unbindKeyDown('Ctrl+D')
  
  if updateEvent ~= nil then
	  removeEvent(updateEvent)
	  updateEvent = nil
  end
end

function onMiniWindowClose()
  debugButton:setOn(false)
end

function toggle()
  if debugButton:isOn() then
    debugWindow:close()
    debugButton:setOn(false)
  else
    debugWindow:open()
    debugButton:setOn(true)
  end
end

function update()
  updateEvent = scheduleEvent(update, 200)
  if not debugWindow:isVisible() then
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
  
  iter = iter + 1
  if iter == 50 then
    iter = 0
    g_stats.clear(1)
    g_stats.clear(2)
    g_stats.clear(3)    
    g_stats.clear(4)    
    g_stats.clear(5)    
  end
end

