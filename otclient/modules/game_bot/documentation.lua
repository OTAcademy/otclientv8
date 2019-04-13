botDocumentation = [[
# OTClientV8 BOT Documentation
Documentation is currently outdated, will update it soon

# Important tips
Item ID is visible after right click on them (from context menu) 
Don't use return in config (lua function)

# Predefined variables
init - true if it's first script call
player 
followed - followed creature, can be null
attacked or target - attacked creature, can be null
hp - current health points
hpmax - max health points
mana - current mana points
manamax - max mana points
pos - player position, struct {x, y, z}
time or now - timestamp in milliseconds
storage - dictionary to store data between calls, more informations bellow


# Storage
Variable storage is used to store data between different bot calls.
It's cleared after every config change in editor, switching config on/off or otclient restart.
Example:
if storage.lastSay == nil or storage.lastSay + 1000 < time then
  storage.lastSay = time
  say("saying something every one second")  
end

# Functions
say(text) - say something by player on default channel, for eg. say("exura")
use(itemid) - use an item that player have
useWith(itemid, target) - use an item that player have on target, for eg. useWith(238, player) for potion
distance(pos1, pos2) - return distance between positions, return dict {x, y, z}

exh(exhId, timeoutInMs) - more informations bellow

setupUi(ui) - ui setup, will be called only when init == true, otherwise ignored, more informations bellow

-- callbacks setup, should be called when init == true, max one callback per combinration, callback can be nil to disable (unbind)

bindKeyUp(key, callback) -- example bindKeyUp('Ctrl+P', function say("lol i wanted print tibia") end)
bindKeyDown(key, callback)
bindKeyPress(key, callback)

# Limiting calls (exhaust)
Because script is run every 50 ms, doing sometimg like say("asd") would result in spamming asd 20 times per second which is not a good idea because you will be muted by server.You need to limit it somehow. First option is to use storage like in example 'saying something every one second', but you can do it easier with exh(exhId, timeoutInMs) function. This function is checking if timeoutInMs has passed since last call exh(exhId, whatever), if yes, returns true, otherwise false.
Here's an example:
if exh(1, 1000) then
  say("saying something every one second")  
end

-- saying exura but not more than once per second 
if hp < hpmax / 2 and exh(2, 1000) then
  say("exura")
end

In if statement exh function should be checked (called) as the last thing

In general, function exh looks like this:
function(id, ms) 
  if storage[id] == nil or storage[id] + ms < now then
    storage[id] = now
    return true
  end
  return false
end

# UI
Todo. You can get access to ui widget by it's id using ui.widgetId. Currently only an example. 

setupUI({
  {id="hello", type="Label",text="Example label of example config"},
  {id="b1",type="Button",text="Example button"},
  {id="counter",type="Label",text=""},
  {id="pos",type="Label",text=""},
  {id="target",type="Label",text=""}
})

ui.b1.onClick = function()
  storage.clicks = storage.clicks + 1
  ui.counter:setText("Buttons clicks: " .. storage.clicks)
end

# Example config
if init then
  storage.clicks = 0
  bindKeyPress('f5', function() say('bot detected f5 press, lol') end)
end

setupUI({
{id="hello", type="Label",text="Example label of example config"},
{id="button",type="Button",text="Example button"},
{id="counter",type="Label",text=""},
{id="pos",type="Label",text=""},
{id="target",type="Label",text=""}
})

ui.button.onClick = function()
  storage.clicks = storage.clicks + 1
  ui.counter:setText("Buttons clicks: " .. storage.clicks)
end

-- auto strong mana potion
if mana < manamax/2 and exh(1, 500) then
  usewith(237, player)
end

-- auto strong health potion
if hp < hpmax/2 and exh(2, 500) then
  usewith(236, player)
end

-- saying something
if exh(5, 5500) then
  talk("[otclient bot] talking something every 5.5s. Time in ms from start: " .. now)
end

-- attacking target every 2 s
if attacked and mana > 50 and exh(7, 2000) then
  talk("exori vis")
end

local playerPos = pos.x .. "," .. pos.y .. "," .. pos.z
local targetPos = "empty target"
if target then
  targetPos = target:getPosition().x .. "," .. target:getPosition().y .. "," .. target:getPosition().z
end
ui.pos:setText(playerPos)
ui.target:setText(targetPos)
]]

