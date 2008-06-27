--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    applyopts.lua
--  brief:   returns a function that applies map options to a mapInfo table
--  author:  Dave Rodgers
--
--  Copyright (C) 2008.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

-- Special table for map config files
--
--   Map.fileName
--   Map.fullName
--   Map.configFile


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

if (not Spring.GetMapOptions) then
  return function (mapInfo)
    -- map options not available (unitsync),
    -- no need to modify the mapInfo table
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local mapOptions = Spring.GetMapOptions()


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function ParseFloat3(value)
  local t = type(value)
  if (t == 'table') then
    if (tonumber(value[1]) and
        tonumber(value[2]) and
        tonumber(value[3])) then
      return value
    else
      return nil
    end
  elseif (t == 'string') then
    local s, e, v1, v2, v3 = value:find('^%s*(%S)%s+(%S)%s+(%S)%s*$')
    v1 = tonumber(v1)
    v2 = tonumber(v2)
    v3 = tonumber(v3)
    if (v1 and v2 and v3) then
      return { v1, v2, v3 }
    else
      return nil
    end
  end
  return nil
end


local function TintColor(color, tint)
  return {
    color[1] * tint[1],
    color[2] * tint[2],
    color[3] * tint[3],
  }
end


local function SetColorComponent(color, component, value)
  color = ParseFloat3(color)
  if (not color) then
    return false
  end

  if ((component == 1) or (component == 'r')) then
    color[1] = tonumber(value)
  elseif ((component == 2) or (component == 'g')) then
    color[2] = tonumber(value)
  elseif ((component == 3) or (component == 'b')) then
    color[3] = tonumber(value)
  else
    return false
  end

  return true
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function UpdateBasics(mapInfo)
  if (mapOptions.voidwater) then
    mapInfo.voidwater = mapOptions.voidwater
  end
  if (mapOptions.notdeformable) then
    mapInfo.notdeformable = mapOptions.notdeformable
  end
  if (mapOptions.hardness) then
    mapInfo.hardness = mapOptions.hardness
  end
  if (mapOptions.gravity) then
    mapInfo.gravity = mapOptions.gravity
  end
  if (mapOptions.extractorradius) then
    mapInfo.extractorradius = mapOptions.extractorradius
  end
  if (mapOptions.tidalstrength) then
    mapInfo.tidalstrength = mapOptions.tidalstrength
  end
end


local function UpdateAtmosphere(mapInfo)
  mapInfo.atmosphere = mapInfo.atmosphere or {}
  local atmo = mapInfo.atmosphere

  if (mapOptions.minwind) then
    atmo.minwind = mapOptions.minwind
  end
  if (mapOptions.maxwind) then
    atmo.maxwind = mapOptions.maxwind
  end
  if (mapOptions.clouddensity) then
    atmo.clouddensity = mapOptions.clouddensity
  end
end



local function UpdateWater(mapInfo)
  mapInfo.water = mapInfo.water or {}
  local water = mapInfo.water

  if (mapOptions.waterdamage) then
    water.damage = mapOptions.waterdamage
  end
end


local function UpdateLighting(mapInfo)
  mapInfo.lighting = mapInfo.lighting or {}
  local lighting = mapInfo.lighting

  if (mapOptions.tint) then
    if (tint == 'dark') then end
  end
  if (mapOptions.grounddiffusecolor_r) then
    lighting.grounddiffusecolor =
      SetColorPart(lighting.grounddiffusecolor, 'r', mapOptions.grounddiffusecolor_r)
  end
end


local function UpdateTerrainTypes(mapInfo)
  mapInfo.terrain = mapInfo.terrain or {}
  local terrain = mapInfo.terrain
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

return function(mapInfo)

  UpdateBasics(mapInfo)

  UpdateAtmosphere(mapInfo)

  UpdateWater(mapInfo)

  UpdateLighting(mapInfo)

  UpdateTerrainTypes(mapInfo)

  if (not mapInfo.description) then
    mapInfo.description = Map.fileName
  end
  mapInfo.description = mapInfo.description .. ' (with MapOptions)'
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
