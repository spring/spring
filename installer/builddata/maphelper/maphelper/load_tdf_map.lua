--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:     load_tdf_map.lua
--  brief:    tdf map lua parser (original SMD and SM3 formats)
--  authors:  Dave Rodgers
--
--  Copyright (C) 2008.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

-- Special table for map config files
--
--   Map.name
--   Map.fullName
--   Map.configFile

local mapConfig = Map.configFile

local TDF = VFS.Include('maphelper/parse_tdf.lua')


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Conversion Helpers
--

local function ConvertMoveSpeeds(terrain)
  local moveSpeeds = {}
  for k, v in pairs(terrain) do
    local s, e, type = k:find('^(.*)movespeed$')
    if (type) then
      moveSpeeds[type] = v
      terrain[k] = nil
    end
  end
  terrain.movespeeds = moveSpeeds
end


local function ConvertTerrainTypes(map)
  local typeArray = {}
  for k, v in pairs(map) do
    if (type(v) == 'table') then
      local s, e, num = k:find('^terraintype(.*)$')
      local num = tonumber(num)
      if (num) then
        ConvertMoveSpeeds(v)
        typeArray[num] = v
        map[k] = nil
      end
    end
  end
  map.terraintypes = typeArray
end


local function ConvertTeams(map)
  local teamArray = {}
  for k, v in pairs(map) do
    local s, e, num = k:find('^team(.*)$')
    local num = tonumber(num)
    if (num) then
      teamArray[num] = {
        startpos = {
          x = v.startposx,
          z = v.startposz,
        },
      }
      map[k] = nil
    end
  end
  map.teams = teamArray
end


local function ConvertLighting(map)
  local light = map.light
  if (type(light) ~= 'table') then
    return
  end
  local lighting = {}
  for k, v in pairs(light) do
    local s, e, prefix = k:find('^(.*)suncolor$')
    if (prefix) then
      k = prefix .. 'diffusecolor'
    end
    lighting[k] = v
  end
  map.light = nil
  map.lighting = lighting
end


local function ConvertWater(map)
  -- remove the 'water' prefix
  local water = map.water
  if (type(water) ~= 'table') then
    return
  end
  for k, v in pairs(water) do
    local s, e, suffix = k:find('^water(.*)$')
    if (suffix) then
      water[suffix] = v
      water[k] = nil
    end
  end
  -- FIXME -- handle caustics?
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Main Routine
--

local mapData, err = TDF.Parse(mapConfig)
if (mapData == nil) then
  error('Error parsing ' .. mapConfig .. ': ' .. err)
end

local map = mapData.map
if (map == nil) then
  error('Error parsing ' .. mapConfig .. ': missing MAP section')
end


ConvertTerrainTypes(map)

ConvertLighting(map)

ConvertWater(map)

ConvertTeams(map)


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Debugging
--

-- FIXME -- debugging
local function PrintTable(t, indent)
  indent = indent or ''
  for k,v in pairs(t) do
    k = tonumber(k) and '['..k..']' or k
    if (type(v) == 'table') then
      print(indent .. k .. ' = {')
      PrintTable(v, indent .. '  ')
      print(indent .. '},')
    else
      print(indent .. k .. ' = "' .. v .. '",')
    end
  end
end

--PrintTable(map)


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

return map

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
