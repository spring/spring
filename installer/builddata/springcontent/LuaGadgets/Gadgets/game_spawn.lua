--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    game_spawn.lua
--  brief:   spawns start unit and sets storage levels
--  author:  Tobi Vollebregt
--
--  Copyright (C) 2010.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function gadget:GetInfo()
	return {
		name      = "Spawn",
		desc      = "spawns start unit and sets storage levels",
		author    = "Tobi Vollebregt",
		date      = "January, 2010",
		license   = "GNU GPL, v2 or later",
		layer     = 0,
		enabled   = true  --  loaded by default?
	}
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function GetStartUnit(teamID)
	-- get the team startup info
	local _,_,_,_,side = Spring.GetTeamInfo(teamID)
	local startUnit
	if (side == "") then
		-- startscript didn't specify a side for this team
		local sidedata = Spring.GetSideData()
		if (sidedata and #sidedata > 0) then
			startUnit = sidedata[1 + teamID % #sidedata].startUnit
		end
	else
		startUnit = Spring.GetSideData(side)
	end
	return startUnit
end


local function SpawnStartUnit(teamID)
	local startUnit = GetStartUnit(teamID)
	if (startUnit and startUnit ~= "") then
		-- spawn the specified start unit
		local x,y,z = Spring.GetTeamStartPosition(teamID)
		local unitID = Spring.CreateUnit(startUnit, x, y, z, 0, teamID)

		-- set the *team's* lineage root
		Spring.SetUnitLineage(unitID, teamID, true)

		-- remove the pre-existing storage
		--   must be done after the start unit is spawned,
		--   otherwise the starting resources are lost!
		local ud = UnitDefs[UnitDefNames[startUnit].id]
		Spring.SetTeamResource(teamID, "ms", ud.metalStorage)
		Spring.SetTeamResource(teamID, "es", ud.energyStorage)
	end
end


function gadget:GameStart()
	local gaiaTeamID = Spring.GetGaiaTeamID()
	local teams = Spring.GetTeamList()
	for i = 1,#teams do
		local teamID = teams[i]
		-- don't spawn a start unit for the Gaia team
		if (teamID ~= gaiaTeamID) then
			SpawnStartUnit(teamID)
		end
	end
end

