botDefaultConfig = {
  configs = {
    {name = "Example", script = [[
--Example
--#ui
setupUI({
{type="Label", text=tr("Macros"), margin={5,0,0,0}},
{id="macros",type="Panel"},
{type="Separator", margin={7,10,5,10}},
{id="hello", type="Label",text="Example label of example config. Press F5 to check hotkeys. Also use magic wall rune"},
{id="button",type="Button",text="Example button, click it"},
{id="counter",type="Label",text=""},
{id="switch", type="Switch", text="example switch", on=true},
{type="Separator"},
{id="pos",type="Label",text=""},
{id="target",type="Label",text=""},
{id="msginfo",type="Label",text="Last text:"},
{id="msg",type="Label",text=""},
{id="ping",type="Label",text=""},
})

ui.switch.onClick = function()
  local on = not ui.switch:isOn()
  ui.switch:setOn(on)
  if on then
    say("switch on")
  else
    say("switch off")
  end
end

ui.button.onClick = function()  
  storage.clicks = storage.clicks + 1
  ui.counter:setText("Clicks: " .. storage.clicks)
end

macro(100, function()
ui.ping:setText("Ping: " .. ping)
end)--#macros
macro(50, function()
if mana < manamax/2 then
  usewith(237, player)
end
end)

macro(50, "health", 'f6', function()
if hp < hpmax/2 then
  usewith(236, player)
end
end)

macro(5500, 'saying every 5.5s', 'f7', function()
  talk("[otclient bot] talking something every 5.5s. Time in ms from start: " .. now)
end)

macro(1000, 'dice', 'f9', function()
  for i=5792,5798 do
    if findItem(i) then
      use(i)
      return
    end
  end
  warn("you need a dice to use it. try to open backpack")
end)


macro(50, 'crazy turning', 'ctrl+f8', function()
   g_game.turn(math.random(1,4))
end)
--#hotkeys
bindKey('f5', function() say('bot detected f5 press, lol') info(now) end)--#loot

--#init
print("script initialization")
info("example info")
warn("example warn")
error("example error")
if storage.clicks == nil then
  storage.clicks = 0
end
--#callbacks
bindOnTalk(function(name, level, mode, text, channelId, pos)
  ui.msg:setText(name .. ":" .. text)
end)--#attack
macro(50, function()
  if attacked and mana > 50 and exh(7, 2000) then
    talk("exori vis")
  end
end)
--#other
macro(100, function() 
local playerPos = pos.x .. "," .. pos.y .. "," .. pos.z
local targetPos = "empty target"
if target then
  targetPos = target:getPosition().x .. "," .. target:getPosition().y .. "," .. target:getPosition().z
end
ui.pos:setText(playerPos)
ui.target:setText(targetPos)
end)

macro(1000, 'changing outfits', function()
    for i, tile in ipairs(getTiles()) do
      for c, creature in ipairs(tile:getCreatures()) do
        if creature:getOutfit() ~= player:getOutfit() then
           creature:setOutfit(player:getOutfit())
        end
      end
    end
end)

macro(100, 'flagging magicwalls', function()
    for i, tile in ipairs(getTiles()) do
      for c, item in ipairs(tile:getItems()) do
        if item:getId() == 2129 then
          local staticText = StaticText.create()
          staticText:addMessage("", 9, "magic wall!")
          g_map.addThing(staticText, tile:getPosition(), -1)
        end
      end
    end
end)

    ]]},
    {name = "HTTP test", script = [[
--HTTP test

storage.players = {}

http.get("https://www.xavato.eu/?subtopic=whoisonline", function(url, data, err)
if err:len() > 0 then
return error("Http error: " .. err)
end
matches =regexMatch(data, "subtopic=characters&name=([^\"]*)")
for i, match in ipairs(matches) do
  local name = match[2]:gsub('+', ' ')
  table.insert(storage.players, name)
  end
info("Targets: " .. #storage.players)
end)
    
macro(1000, function()
  if #storage.players == 0 then
    return
  end
  say(storage.players[1])
  -- sayPrivate(name, text)
  table.remove(storage.players, 1)
end)
]]},
  {}, {}, {}
  },
  enabled = false,
  selectedConfig = 1
}