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
  local remove = {}
  for k, v in pairs(terrain) do
    local s, e, type = k:find('^(.*)movespeed$')
    if (type) then
      moveSpeeds[type] = v
      remove[#remove+1] = k
    end
  end
  for i=1,#remove do
    terrain[remove[i]] = nil
  end
  terrain.movespeeds = moveSpeeds
end


local function ConvertTerrainTypes(map)
  local typeArray = {}
  local remove = {}
  for k, v in pairs(map) do
    if (type(v) == 'table') then
      local s, e, num = k:find('^terraintype(.*)$')
      local num = tonumber(num)
      if (num) then
        ConvertMoveSpeeds(v)
        typeArray[num] = v
        remove[#remove+1] = k
      end
    end
  end
  for i=1,#remove do
    map[remove[i]] = nil
  end
  map.terraintypes = typeArray
end


local function ConvertTeams(map)
  local teamArray = {}
  local remove = {}
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
      remove[#remove+1] = k
    end
  end
  for i=1,#remove do
    map[remove[i]] = nil
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
    -- k = k:gsub("ambiant", "ambient")
    local s, e, prefix = k:find('^(.*)suncolor$')
    if (prefix) then
      k = prefix .. 'diffusecolor'
    end
    lighting[k] = v
  end
  map.light = nil
  map.lighting = lighting
end

local function ConvertSplats(map)
  local splats = map.splats

  if (type(splats) ~= 'table') then
    return
  end

  local csplats = {
    texScales = splats.splattexscales,
    texMults  = splats.splattexmults,
  }

  map.splats = csplats
end

local function ConvertGrass(map)
  local grass = map.grass

  if (type(grass) ~= 'table') then
    return
  end

  local cgrass = {
    bladeWaveScale = grass.grassbladewavescale,
    bladeWidth     = grass.grassbladewidth,
    bladeHeight    = grass.grassbladeheight,
    bladeAngle     = grass.grassbladeangle,
  }

  map.grass = cgrass
end

local function ConvertWater(map)
  -- remove the 'water' prefix
  local water = map.water
  if (type(water) ~= 'table') then
    return
  end

  local new = {}
  map.water = new

  for k, v in pairs(water) do
    local s, e, suffix = k:find('^water(.*)$')
    if (suffix) then
      new[suffix] = v
    else
      new[k] = v
    end
  end
  -- FIXME -- handle caustics?
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
local print = Spring.Echo
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


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Main Routine
--

return function(sourceText)
  local mapData, err = TDF.ParseText(sourceText)
  if (mapData == nil) then
    error('Error parsing ' .. mapConfig .. ': ' .. err)
  end

  local map = mapData.map
  if (map == nil) then
    error('Error parsing ' .. mapConfig .. ': missing MAP section')
  end

  map.resources = {
     detailTex            = map.detailtex,
     specularTex          = map.speculartex,
     splatDetailTex       = map.splatdetailtex,
     splatDistrTex        = map.splatdistrtex,
     grassBladeTex        = map.grassbladetex,
     grassShadingTex      = map.grassshadingtex,
     skyReflectModTex     = map.skyreflectmodtex,
     detailNormalTex      = map.detailnormaltex,
     lightEmissionTex     = map.lightemissiontex,
     parallaxHeightTex    = map.parallaxheighttex,
     splatDetailNormalTex = map.splatdetailnormaltex, -- table
  }

  ConvertTerrainTypes(map)
  ConvertLighting(map)
  ConvertSplats(map)
  ConvertGrass(map)
  ConvertWater(map)
  ConvertTeams(map)

  --PrintTable(map)  -- FIXME -- debugging

  return map
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
