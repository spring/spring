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

local modRules = {}


local haveRules   = false
local haveSensors = false
local section =  'modrules.lua'


if (VFS.FileExists('gamedata/modrules.tdf')) then
	local err
	modRules, err = TDF.Parse('gamedata/modrules.tdf')
	if (modRules == nil) then
		Spring.Log(section, LOG.ERROR, 'Error parsing modrules.tdf: ' .. err)
	end
	haveRules = true
end


if (VFS.FileExists('gamedata/sensors.tdf')) then
	local sensors, err = TDF.Parse('gamedata/sensors.tdf')
	if (sensors == nil)  then
		Spring.Log(section, LOG.ERROR, 'Error parsing sensors.tdf: ' .. err)
	end
	modRules.sensors = sensors
	haveSensors = true
end


if (not (haveRules or haveSensors)) then
	return {}
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

return modRules

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
