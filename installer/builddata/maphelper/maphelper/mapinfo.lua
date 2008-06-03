--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    mapinfo.lua
--  brief:   map configuration loader
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

local function IsTdf(text)
  local s, e, c = text:find('(%S)') -- first non-space character
  -- TDF files should start with:
  --   '[name]' - section header
  --   '/*'     - comment
  --   '//      - comment
  return ((c == '[') or (c == '/'))
end


--------------------------------------------------------------------------------

local configFile = Map.configFile

local configSource, err = VFS.LoadFile(configFile)
if (configSource == nil) then
  error(configFile .. ': ' .. err)
end


--------------------------------------------------------------------------------

local mapInfo, err

if (IsTdf(configSource)) then
  local mapTdfParser = VFS.Include('maphelper/parse_tdf_map.lua')
  mapInfo, err = mapTdfParser(configSource)
  if (mapInfo == nil) then
    error(configFile .. ': ' .. err)
  end
else
  local chunk, err = loadstring(configSource, configFile)
  if (chunk == nil) then
    error(configFile .. ': ' .. err)
  end
  mapInfo = chunk()
end


--------------------------------------------------------------------------------

if (Spring.GetMapOptions and mapInfo.autooptions) then
  local optsFunc = VFS.Include('maphelper/applyopts.lua')
  optsFunc(mapInfo)
end


--------------------------------------------------------------------------------

return mapInfo

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
