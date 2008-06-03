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

local function TintColor(color, tint)
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
