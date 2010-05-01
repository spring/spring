--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    MapOptions.lua
--  brief:   default MapOptions file
--  author:  Dave Rodgers
--
--  Copyright (C) 2008.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local mapInfo = VFS.Include('maphelper/mapinfo.lua')
if (not mapInfo.defaultoptions) then
  return {} -- only load the default options if they've been requested
end

local setupFunc = VFS.Include('maphelper/setupopts.lua')
return setupFunc(mapInfo)

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
