function InputMessage:getData()
  local dataType = self:getU8()
  if dataType == NetworkMessageTypes.Boolean then
    return numbertoboolean(self:getU8())
  elseif dataType == NetworkMessageTypes.U8 then
    return self:getU8()
  elseif dataType == NetworkMessageTypes.U16 then
    return self:getU16()
  elseif dataType == NetworkMessageTypes.U32 then
    return self:getU32()
  elseif dataType == NetworkMessageTypes.U64 then
    return self:getU64()
  elseif dataType == NetworkMessageTypes.NumberString then
    return tonumber(self:getString())
  elseif dataType == NetworkMessageTypes.String then
    return self:getString()
  elseif dataType == NetworkMessageTypes.Table then
    return self:getTable()
  else
    perror('Unknown data type ' .. dataType)
  end
  return nil
end

function InputMessage:getTable()
  local ret = {}
  local size = self:getU16()
  for i=1,size do
    local index = self:getData()
    local value = self:getData()
    ret[index] = value
  end
  return ret
end

function InputMessage:getColor()
  local color = {}
  color.r = self:getU8()
  color.g = self:getU8()
  color.b = self:getU8()
  color.a = self:getU8()
  return color
end

function InputMessage:getPosition()
  local position = {}
  position.x = self:getU16()
  position.y = self:getU16()
  position.z = self:getU8()
  return position
end

function InputMessage:getOutfit()
  local outfit = {}
  local lookType

  if g_game.getFeature(GameLooktypeU16) then
    lookType = self:getU16()
  else
    lookType = self:getU8()
  end

  if (lookType ~= 0) then
    local lookHead = self:getU8()
    local lookBody = self:getU8()
    local lookLegs = self:getU8()
    local lookFeet = self:getU8()
    local lookAddons = 0

    if g_game.getFeature(GamePlayerAddons) then
      lookAddons = self:getU8()
    end

    outfit.type = lookType
    outfit.feet = lookFeet
    outfit.addons = lookAddons
    outfit.legs = lookLegs
    outfit.head = lookHead
    outfit.body = lookBody
  else
    outfit.auxType = self:getU16()
  end

  if g_game.getFeature(GamePlayerMounts) then
    outfit.mount = self:getU16()
  end

  if g_game.getFeature(GameOutfitShaders) then
    outfit.shader = self:getString()
  end

  return outfit
end
