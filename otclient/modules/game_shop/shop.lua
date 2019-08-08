-- private variables
local SHOP_EXTENTED_OPCODE = 201

local shop
local available = false
local shopButton = nil

local function sendAction(action, data)
  local protocolGame = g_game.getProtocolGame()
  if data == nil then
    data = {}
  end
  if protocolGame then
    protocolGame:sendExtendedOpcode(SHOP_EXTENTED_OPCODE, json.encode({action = action, data = data}))
  end  
end

-- public functions
function init()
  shop = g_ui.displayUI('shop')
  shop:hide()

  connect(g_game, {  onGameStart = check, onGameEnd = hide  })

  ProtocolGame.registerExtendedOpcode(SHOP_EXTENTED_OPCODE, onExtendedOpcode)

  if g_game.isOnline() then
    check()
  end
end

function terminate()
  disconnect(g_game, {  onGameEnd = hide  })

  ProtocolGame.unregisterExtendedOpcode(SHOP_EXTENTED_OPCODE, onExtendedOpcode)
  
  if shopButton then
    shopButton:destroy()
    shopButton = nil
  end
  shop:destroy()
  shop = nil
end

function check()
  if not g_game.getFeature(GameExtendedOpcode) then
    return
  end
  sendAction("info")
end

function hide()
  shop:hide()
end

function show()
  if not available then
    return
  end
  shop:show()
end

function toggle()
  if shop:isVisible() then
    return hide()
  end
  show()
end

function onExtendedOpcode(protocol, code, buffer)
  if not available then
    available = true
    shopButton = modules.client_topmenu.addRightGameToggleButton('shopButton', tr('Shop'), '/images/topbuttons/shop', toggle)
  end

  print(buffer)

  local status, json_data = pcall(function() return json.decode(buffer) end)
  if not status then
    return false
  end

  local action = json_data['action']
  local data = json_data['data']
  if not action or not data then
    return false
  end

end
