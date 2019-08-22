local proxyPanel = nil
local updateEvent = nil

function init()  
  if not g_proxy then
    return
  end

  proxyPanel = g_ui.loadUI('proxy', modules.game_interface.getMapPanel())
  proxyPanel:hide()
  update()
end

function terminate()
  if not g_proxy then
    return
  end

  removeEvent(updateEvent)

  proxyPanel:destroy()
  proxyPanel = nil
end

function hide()
  if not g_proxy then
    return
  end

  proxyPanel:hide()
end

function show()
  if not g_proxy then
    return
  end

  proxyPanel:show()
end

function update()
  updateEvent = scheduleEvent(update, 100)
  if proxyPanel:isHidden() then
    return
  end
  local text = "Proxies:"
  local proxies = g_proxy.getProxies()
  for proxy_name, proxy_ping in pairs(proxies) do
    text = text .. "\n" .. proxy_name .. " - " .. proxy_ping 
  end
  text = text .. "\n"
  local proxiesDebug = g_proxy.getProxiesDebugInfo()
  for proxy_name, proxy_debug in pairs(proxiesDebug) do
    text = text .. "\n\n" .. proxy_name .. "\n" .. proxy_debug 
  end
  proxyPanel:setText(text)
end