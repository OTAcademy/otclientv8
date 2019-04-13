function executeBot(config, storage, panel)
  local context = {}
  context.panel = panel
  context.storage = storage
  if context.storage.macros == nil then
    context.storage.macros = {}
  end

  -- 
  context.macros = {}
  context.callbacks = {
    onKeyDown = {},
    onKeyUp = {},
    onKeyPress = {},
    onTalk = {},
    onGet = {},
  }
  context.ui = {}
  context.messages = {}

  -- basic functions
  context.print = print
  context.pairs = pairs
  context.ipairs = ipairs
  context.tostring = tostring
  context.math = math
  context.table = table
  context.string = string
  context.tr = tr
  context.json = json
  context.regexMatch = regexMatch

  -- game functions
  context.say = g_game.talk
  context.talk = g_game.talk
  context.talkPrivate = randomFunction2
  context.sayPrivate = context.talkPrivate
  context.use = g_game.useInventoryItem
  context.usewith = g_game.useInventoryItemWith
  context.useWith = g_game.useInventoryItemWith
  context.findItem = g_game.findItemInContainers
  context.g_game = g_game
  context.g_map = g_map
  context.StaticText = StaticText

  -- log functions
  context.log = function(category, text) 
    local query = {added = context.now}
    query[category] = tostring(text)
    for i, msg in ipairs(context.messages) do
      if msg[category] == query[category] then
        msg.added = context.now
        return
      end
    end
    table.insert(context.messages, query)
    if #context.messages > 5 then
      table.remove(context.messages, 1)
    end
  end
  context.info = function(text) return context.log("info", text) end
  context.warn = function(text) return context.log("warn", text) end
  context.error = function(text) return context.log("error", text) end
  context.warning = context.warn

  -- main bot functions
  context.macro = function(timeout, name, hotkey, callback)
    if type(timeout) ~= 'number' or timeout < 1 then
      error("Invalid timeout for macro: " .. tostring(timeout))
    end
    if type(name) == 'function' then
      callback = name
      name = ""
      hotkey = ""
    elseif type(hotkey) == 'function' then
      callback = hotkey
      hotkey = ""
    elseif type(callback) ~= 'function' then
      error("Invalid callback for macro: " .. tostring(callback))
    end
    if type(name) ~= 'string' or type(hotkey) ~= 'string' then
      error("Invalid name or hotkey for macro")
    end

    table.insert(context.macros, {
      timeout = timeout,
      name = name,
      callback = callback,
      lastExecution = context.now
    })
    
    if name:len() > 0 then
      if hotkey:len() > 0 then

      end
    end
  end

  context.setupUI = function(ui, parent) 
    if not parent then
      panel = context.panel
    end

  end

  -- context
  local updateContext = function()
    context.now = g_clock.millis()
    context.time = g_clock.millis()
    context.player = g_game.getLocalPlayer()
    context.followed = g_game.getFollowingCreature()
    context.attacked = g_game.getAttackingCreature()
    context.hp = context.player:getHealth()
    context.maxhp = context.player:getMaxHealth()
    context.hpmax = context.player:getMaxHealth()
    context.mana = context.player:getMana()
    context.manamax = context.player:getMaxMana()
    context.maxmana = context.player:getMaxMana()
    context.pos = context.player:getPosition()
    context.position = context.player:getPosition()
    context.containers = g_game.getContainers()
    context.ping = g_game.getPing()
  end

  -- init context
  updateContext()
  
  -- run script
  assert(load(config, nil, nil, context))()

  return {
    script = function()
      updateContext()
      
      for i, macro in ipairs(context.macros) do
        if (macro.name == nil or macro.name:len() < 1 or context.storage.macros[macro.name]) and macro.lastExecution + macro.timeout <= context.now then
          if macro.callback() ~= false then
            macro.lastExecution = context.now
          end
        end
      end
      
      if #context.messages > 0 then
        if context.messages[1].added + 5000 < context.now then
          table.remove(context.messages, 1)
        end
      end
      return context.messages
    end,
    callbacks = {
      onKeyDown = function(keyCode, keyboardModifiers)

      end,
      onKeyUp = function(keyCode, keyboardModifiers)

      end,
      onKeyPress = function(keyCode, keyboardModifiers, autoRepeatTicks)

      end,
      onTalk = function(name, level, mode, text, channelId, pos)

      end,
      onGet = function(oprationId, url, err, data)

      end
    }    
  }
end