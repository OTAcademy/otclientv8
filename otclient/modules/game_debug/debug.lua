debugWindow = nil
debugButton = nil
local luaStats = nil
local luaCallback = nil
local render = nil
local adaptiveRender = nil

local updateEvent = nil

function init()
  debugButton = modules.client_topmenu.addLeftButton('debugButton',
    'Debug Info', '/images/topbuttons/debug', toggle)
  debugButton:setOn(false)

  debugWindow = g_ui.loadUI('debug', modules.game_interface.getRightPanel())
    
  g_keyboard.bindKeyDown('Ctrl+D', toggle)

  luaStats = debugWindow:recursiveGetChildById('luaStats')
  luaCallback = debugWindow:recursiveGetChildById('luaCallback')
  render = debugWindow:recursiveGetChildById('render')
  adaptiveRender = debugWindow:recursiveGetChildById('adaptiveRender')
  
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
  
  local adaptive = "Adaptive: " .. g_app.getAdaptiveRendererLevel() .. " | " .. g_app.getAdaptiveRendererAvg()
  adaptiveRender:setText(adaptive)
  render:setText(g_extras.getFrameRenderDebufInfo())  
  luaStats:setText(g_stats.get(5))
  luaCallback:setText(g_stats.getCallback(5))
end

