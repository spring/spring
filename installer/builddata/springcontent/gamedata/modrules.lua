--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    modrules.lua
--  brief:   modrules.tdf and sensors.tdf lua parser
--  author:  Dave Rodgers, Craig Lawrence
--  notes:   Spring.GetModOptions() is available
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local TDF = VFS.Include('gamedata/parse_tdf.lua')

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

if (not VFS.FileExists('gamedata/modrules.tdf')) then
  return false
end

local modrules, err = TDF.Parse('gamedata/modrules.tdf')
local sensors, err2 = TDF.Parse('gamedata/sensors.tdf')
if (modrules == nil) then
  error('Error parsing modrules.tdf: ' .. err)
end
if (sensors == nil and VFS.FileExists('gamedata/sensors.tdf'))
	error('Error parsing sensors.tdf: ' .. err2)
end

modrules['sensors'] = sensors
--------------------------------------------------------------------------------

return modrules

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
