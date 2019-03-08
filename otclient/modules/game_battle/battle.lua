battleWindow = nil
battleButton = nil
battlePanel = nil
filterPanel = nil
toggleFilterButton = nil
creatureAgeList = {}
battleButtonsList = {}

mouseWidget = nil

sortTypeBox = nil
sortOrderBox = nil
hidePlayersButton = nil
hideNPCsButton = nil
hideMonstersButton = nil
hideSkullsButton = nil
hidePartyButton = nil

updateEvent = nil

function init()  
  g_ui.importStyle('battlebutton')
  battleButton = modules.client_topmenu.addRightGameToggleButton('battleButton', tr('Battle') .. ' (Ctrl+B)', '/images/topbuttons/battle', toggle)
  battleButton:setOn(true)
  battleWindow = g_ui.loadUI('battle', modules.game_interface.getRightPanel())
  g_keyboard.bindKeyDown('Ctrl+B', toggle)

  -- this disables scrollbar auto hiding
  local scrollbar = battleWindow:getChildById('miniwindowScrollBar')
  scrollbar:mergeStyle({ ['$!on'] = { }})

  battlePanel = battleWindow:recursiveGetChildById('battlePanel')

  filterPanel = battleWindow:recursiveGetChildById('filterPanel')
  toggleFilterButton = battleWindow:recursiveGetChildById('toggleFilterButton')

  if isHidingFilters() then
    hideFilterPanel()
  end

  sortTypeBox = battleWindow:recursiveGetChildById('sortTypeBox')
  sortOrderBox = battleWindow:recursiveGetChildById('sortOrderBox')
  hidePlayersButton = battleWindow:recursiveGetChildById('hidePlayers')
  hideNPCsButton = battleWindow:recursiveGetChildById('hideNPCs')
  hideMonstersButton = battleWindow:recursiveGetChildById('hideMonsters')
  hideSkullsButton = battleWindow:recursiveGetChildById('hideSkulls')
  hidePartyButton = battleWindow:recursiveGetChildById('hideParty')

  mouseWidget = g_ui.createWidget('UIButton')
  mouseWidget:setVisible(false)
  mouseWidget:setFocusable(false)
  mouseWidget.cancelNextRelease = false

  battleWindow:setContentMinimumHeight(80)

  sortTypeBox:addOption('Name', 'name')
  sortTypeBox:addOption('Distance', 'distance')
  sortTypeBox:addOption('Age', 'age')
  sortTypeBox:addOption('Health', 'health')
  sortTypeBox:setCurrentOptionByData(getSortType())
  sortTypeBox.onOptionChange = onChangeSortType

  sortOrderBox:addOption('Asc.', 'asc')
  sortOrderBox:addOption('Desc.', 'desc')
  sortOrderBox:setCurrentOptionByData(getSortOrder())
  sortOrderBox.onOptionChange = onChangeSortOrder

  updateBattleList()
  battleWindow:setup()
  
  connect(LocalPlayer, {
    onPositionChange = onCreaturePositionChange
  })
end

function terminate()
  if battleButton == nil then
	return
  end
  
  g_keyboard.unbindKeyDown('Ctrl+B')
  battleButtonsByCreaturesList = {}
  battleButton:destroy()
  battleWindow:destroy()
  mouseWidget:destroy()
	
  disconnect(LocalPlayer, {
    onPositionChange = onCreaturePositionChange
  })

  if updateEvent ~= nil then
	  removeEvent(updateEvent)
	  updateEvent = nil
  end
end

function toggle()
  if battleButton:isOn() then
    battleWindow:close()
    battleButton:setOn(false)
  else
    battleWindow:open()
    battleButton:setOn(true)
  end
end

function onMiniWindowClose()
  battleButton:setOn(false)
end

function getSortType()
  local settings = g_settings.getNode('BattleList')
  if not settings then
    return 'name'
  end
  return settings['sortType']
end

function setSortType(state)
  settings = {}
  settings['sortType'] = state
  g_settings.mergeNode('BattleList', settings)

  checkCreatures()
end

function getSortOrder()
  local settings = g_settings.getNode('BattleList')
  if not settings then
    return 'asc'
  end
  return settings['sortOrder']
end

function setSortOrder(state)
  settings = {}
  settings['sortOrder'] = state
  g_settings.mergeNode('BattleList', settings)

  checkCreatures()
end

function isSortAsc()
    return getSortOrder() == 'asc'
end

function isSortDesc()
    return getSortOrder() == 'desc'
end

function isHidingFilters()
  local settings = g_settings.getNode('BattleList')
  if not settings then
    return false
  end
  return settings['hidingFilters']
end

function setHidingFilters(state)
  settings = {}
  settings['hidingFilters'] = state
  g_settings.mergeNode('BattleList', settings)
end

function hideFilterPanel()
  filterPanel.originalHeight = filterPanel:getHeight()
  filterPanel:setHeight(0)
  toggleFilterButton:getParent():setMarginTop(0)
  toggleFilterButton:setImageClip(torect("0 0 21 12"))
  setHidingFilters(true)
  filterPanel:setVisible(false)
end

function showFilterPanel()
  toggleFilterButton:getParent():setMarginTop(5)
  filterPanel:setHeight(filterPanel.originalHeight)
  toggleFilterButton:setImageClip(torect("21 0 21 12"))
  setHidingFilters(false)
  filterPanel:setVisible(true)
end

function toggleFilterPanel()
  if filterPanel:isVisible() then
    hideFilterPanel()
  else
    showFilterPanel()
  end
end

function onChangeSortType(comboBox, option)
  setSortType(option:lower())
end

function onChangeSortOrder(comboBox, option)
  -- Replace dot in option name
  setSortOrder(option:lower():gsub('[.]', ''))
end

-- functions
function updateBattleList() 
	updateEvent = scheduleEvent(updateBattleList, 200)
  checkCreatures()
end

function checkCreatures()
  if not g_game.isOnline() then
    return
  end

  local player = g_game.getLocalPlayer()
  local spectators = g_map.getSpectators(player:getPosition(), false)
  
  creatures = {}
  for _, creature in ipairs(spectators) do
    if doCreatureFitFilters(creature) then
      table.insert(creatures, creature)	
      if creatureAgeList[creature] == nil then
        creatureAgeList[creature] = os.time()
      end
    end
  end
  
  local following = g_game.getAttackingCreature()
  local attacking = g_game.getAttackingCreature()
  
  -- sorting
  local creature_i = 0
  sortCreatures(creatures)
  for i=1, #creatures do
	  creature_i = i
	  local battleButton = battleButtonsList[creature_i]
	  if battleButton == nil then
	    battleButton = g_ui.createWidget('BattleButton')
      battleButton.onHoverChange = onBattleButtonHoverChange
      battleButton.onMouseRelease = onBattleButtonMouseRelease
      table.insert(battleButtonsList, battleButton)
      battlePanel:addChild(battleButton)
 	  end
	  
	  local creature = creatures[creature_i]
	  if isSortAsc() then
      creature = creatures[#creatures - creature_i + 1]
	  end
	  battleButton:newSetup(creature)
	  
	  local isTarget = creature == attacking
	  local isFollowed = creature == following
	  if isTarget ~= battleButton.isTarget or isFollowed ~= battleButton.isFollowed then
		  battleButton.isTarget = isTarget
		  battleButton.isFollowed = isFollowed
		  battleButton:update(creature)
	  end
	  
	  battleButton:setVisible(player:hasSight(creature:getPosition()) and creature:canBeSeen())
  end
  creature_i = creature_i + 1
  while creature_i <= #battleButtonsList do
    battleButtonsList[creature_i]:setVisible(false)
    creature_i = creature_i + 1
  end
end

function doCreatureFitFilters(creature)
  if creature:isLocalPlayer() then
    return false
  end

  local pos = creature:getPosition()
  if not pos then return false end

  local localPlayer = g_game.getLocalPlayer()
  if pos.z ~= localPlayer:getPosition().z or not creature:canBeSeen() then return false end

  local hidePlayers = hidePlayersButton:isChecked()
  local hideNPCs = hideNPCsButton:isChecked()
  local hideMonsters = hideMonstersButton:isChecked()
  local hideSkulls = hideSkullsButton:isChecked()
  local hideParty = hidePartyButton:isChecked()

  if hidePlayers and creature:isPlayer() then
    return false
  elseif hideNPCs and creature:isNpc() then
    return false
  elseif hideMonsters and creature:isMonster() then
    return false
  elseif hideSkulls and creature:isPlayer() and creature:getSkull() == SkullNone then
    return false
  elseif hideParty and creature:getShield() > ShieldWhiteBlue then
    return false
  end

  return true
end

local function getDistanceBetween(p1, p2)
    return math.max(math.abs(p1.x - p2.x), math.abs(p1.y - p2.y))
end

function sortCreatures(creatures)
  local player = g_game.getLocalPlayer()
  
  if getSortType() == 'distance' then
	local playerPos = player:getPosition()
    table.sort(creatures, function(a, b) return getDistanceBetween(playerPos, a:getPosition()) > getDistanceBetween(playerPos, b:getPosition()) end)
  elseif getSortType() == 'name' then
    table.sort(creatures, function(a, b) return a:getName():lower() > b:getName():lower() end)
  elseif getSortType() == 'health' then
    table.sort(creatures, function(a, b) return a:getHealthPercent() > b:getHealthPercent() end)
  elseif getSortType() == 'age' then
    table.sort(creatures, function(a, b) return creatureAgeList[a] > creatureAgeList[b] end)
  end
end

-- other functions
function onBattleButtonMouseRelease(self, mousePosition, mouseButton)
  if mouseWidget.cancelNextRelease then
    mouseWidget.cancelNextRelease = false
    return false
  end
  if ((g_mouse.isPressed(MouseLeftButton) and mouseButton == MouseRightButton)
    or (g_mouse.isPressed(MouseRightButton) and mouseButton == MouseLeftButton)) then
    mouseWidget.cancelNextRelease = true
    g_game.look(self.creature, true)
    return true
  elseif mouseButton == MouseLeftButton and g_keyboard.isShiftPressed() then
    g_game.look(self.creature, true)
    return true
  elseif mouseButton == MouseRightButton and not g_mouse.isPressed(MouseLeftButton) then
    modules.game_interface.createThingMenu(mousePosition, nil, nil, self.creature)
    return true
  elseif mouseButton == MouseLeftButton and not g_mouse.isPressed(MouseRightButton) then
    if self.isTarget then
      g_game.cancelAttack()
    else
      g_game.attack(self.creature)
    end
    return true
  end
  return false
end

function onBattleButtonHoverChange(battleButton, hovered)
  if battleButton.isHovered ~= hovered then
    battleButton.isHovered = hovered
    battleButton:update()
  end
end

function onCreaturePositionChange(creature, newPos, oldPos)
  if creature:isLocalPlayer() then
    if oldPos and newPos and newPos.z ~= oldPos.z then
      checkCreatures()
    end
  end
end