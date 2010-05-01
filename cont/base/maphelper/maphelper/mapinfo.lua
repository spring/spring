--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    mapinfo.lua
--  brief:   map configuration loader
--  author:  Dave Rodgers
--
--  Copyright (C) 2008-2009
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--  Modified to be able to run standalone by Tobi Vollebregt.
--
--  Usage: lua maphelper/mapinfo.lua <configFile>
--         where configFile is the map's .smd file.
--
--  Note that it only runs correctly from the particular directory from which
--  this file can be reached using the 'maphelper/mapinfo.lua' relative path.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

-- Special table for map config files
--
--   Map.fileName
--   Map.fullName
--   Map.configFile

if (not Map) then
  -- Initialize Map table if we're running standalone.
  Map = {configFile = assert(..., "missing configFile argument")}
  print ("parsing " .. Map.configFile)

  -- Create mock VFS table if we're running standalone.
  VFS = {}
  function VFS.Include(filename)
    local chunk = assert(loadfile(filename))
    return chunk()
  end
  function VFS.LoadFile(filename)
    local f = assert(io.open(filename, "r"))
    local r = f:read("*all")
    f:close()
    return r
  end

  -- Create mock Spring table if we're running standalone.
  Spring = {Echo = print}
end

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

if (Spring.GetMapOptions and mapInfo.defaultoptions) then
  local optsFunc = VFS.Include('maphelper/applyopts.lua')
  optsFunc(mapInfo)
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

return mapInfo

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
