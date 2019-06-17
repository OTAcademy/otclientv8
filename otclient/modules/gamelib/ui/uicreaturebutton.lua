-- @docclass
UICreatureButton = extends(UIWidget, "UICreatureButton")

local CreatureButtonColors = {
  onIdle = {notHovered = '#888888', hovered = '#FFFFFF' },
  onTargeted = {notHovered = '#FF0000', hovered = '#FF8888' },
  onFollowed = {notHovered = '#00FF00', hovered = '#88FF88' }
}

local LifeBarColors = {} -- Must be sorted by percentAbove
table.insert(LifeBarColors, {percentAbove = 92, color = '#00BC00' } )
table.insert(LifeBarColors, {percentAbove = 60, color = '#50A150' } )
table.insert(LifeBarColors, {percentAbove = 30, color = '#A1A100' } )
table.insert(LifeBarColors, {percentAbove = 8, color = '#BF0A0A' } )
table.insert(LifeBarColors, {percentAbove = 3, color = '#910F0F' } )
table.insert(LifeBarColors, {percentAbove = -1, color = '#850C0C' } )

function UICreatureButton.create()
  local button = UICreatureButton.internalCreate()
  button:setFocusable(false)
  button.creature = nil
  button.isHovered = false
  return button
end

function UICreatureButton:setCreature(creature)
    self.creature = creature
end

function UICreatureButton:getCreature()
  return self.creature
end

function UICreatureButton:getCreatureId()
    return self.creature:getId()
end

function UICreatureButton:setup(creature)
  self.lifeBarWidget = self:getChildById('lifeBar')
  self.creatureWidget = self:getChildById('creature')
  self.labelWidget = self:getChildById('label')
  self.skullWidget = self:getChildById('skull')
  self.emblemWidget = self:getChildById('emblem')

  self.creature = creature

  self.labelWidget:setText(creature:getName())
  self.creatureWidget:setCreature(creature)

  self:setId('CreatureButton_' .. creature:getName():gsub('%s','_'))
  self:setLifeBarPercent(creature:getHealthPercent())

  self:updateSkull(creature:getSkull())
  self:updateEmblem(creature:getEmblem())
end

function UICreatureButton:update()
  local color = CreatureButtonColors.onIdle
  local show = false
  if self.creature == g_game.getAttackingCreature() then
    color = CreatureButtonColors.onTargeted
    show = true
  elseif self.creature == g_game.getFollowingCreature() then
    color = CreatureButtonColors.onFollowed
    show = true
  end
  color = self.isHovered and color.hovered or color.notHovered

  if self.isHovered or show then
    self.creatureWidget:setBorderWidth(1)
    self.creatureWidget:setBorderColor(color)
    self.labelWidget:setColor(color)
  else
    self.creatureWidget:setBorderWidth(0)
    self.labelWidget:setColor(color)
  end
end

function UICreatureButton:newSetup(creature)
	if self.creature ~= creature then
		self.creature = creature
		self.creatureWidget:setCreature(creature)	
		self.labelWidget:setText(creature:getName())
	end

	self:setLifeBarPercent(creature:getHealthPercent())
	self:updateSkull(creature:getSkull())
	self:updateEmblem(creature:getEmblem())
  self:update()
end

function UICreatureButton:updateSkull(skullId)
  if not self.creature then
    return
  end
  local skullId = skullId or self.creature:getSkull()

  if skullId ~= SkullNone then
    self.skullWidget:setWidth(self.skullWidget:getHeight())
    local imagePath = getSkullImagePath(skullId)
    self.skullWidget:setImageSource(imagePath)
    self.labelWidget:setMarginLeft(5)
  else
    self.skullWidget:setWidth(0)
    if self.creature:getEmblem() == EmblemNone then
      self.labelWidget:setMarginLeft(2)
    end
  end
end

function UICreatureButton:updateEmblem(emblemId)
  if not self.creature then
    return
  end
  local emblemId = emblemId or self.creature:getEmblem()
  if emblemId ~= EmblemNone then
    self.emblemWidget:setWidth(self.emblemWidget:getHeight())
    local imagePath = getEmblemImagePath(emblemId)
    self.emblemWidget:setImageSource(imagePath)
    self.emblemWidget:setMarginLeft(5)
    self.labelWidget:setMarginLeft(5)
  else
    self.emblemWidget:setWidth(0)
    self.emblemWidget:setMarginLeft(0)
    if self.creature:getSkull() == SkullNone then
      self.labelWidget:setMarginLeft(2)
    end
  end
end

function UICreatureButton:setLifeBarPercent(percent)
  self.lifeBarWidget:setPercent(percent)

  local color
  for i, v in pairs(LifeBarColors) do
    if percent > v.percentAbove then
      color = v.color
      break
    end
  end

  self.lifeBarWidget:setBackgroundColor(color)
end