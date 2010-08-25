--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    game_end.lua
--  brief:   spawns start unit and sets storage levels
--  author:  Andrea Piras
--
--  Copyright (C) 2010.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function gadget:GetInfo()
	return {
		name      = "Game End",
		desc      = "Handles team/allyteam deaths and declares gameover",
		author    = "Andrea Piras",
		date      = "August, 2010",
		license   = "GNU GPL, v2 or later",
		layer     = 0,
		enabled   = true  --  loaded by default?
	}
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

-- synced only
if (not gadgetHandler:IsSyncedCode()) then
	return false
end

local modOptions = Spring.GetModOptions()
local KillTeam = Spring.KillTeam
local GetAllyTeamList = Spring.GetAllyTeamList
local GetTeamList = Spring.GetTeamList
local GameOver = Spring.GameOver
local AreTeamsAllied = Spring.AreTeamsAllied

local teamDieOnZeroUnits = modOptions.teamDieOnZeroUnits or 0
local allyTeamDieOnZeroUnits = modOptions.allyTeamDieOnZeroUnits or 0
local sharedDynamicAllianceVectory = modOptions.sharedDynamicAllianceVectory or 0

local teamsUnitCount = {}
local allyTeamUnitCount = {}
local allyTeamAliveTeamsCount = {}
local teamToAllyTeam = {}

function gadget:TeamDied(teamID)
	local allyTeamID = teamToAllyTeam[teamID]
	if allyTeamID then
		local aliveTeamCount = allyTeamAliveTeamsCount[allyTeamID]
		if aliveTeamCount then
			aliveTeamCount = aliveTeamCount -1
			allyTeamAliveTeamsCount[allyTeamID] = aliveTeamCount
			if aliveTeamCount == 0 then -- one allyteam just died, check the others whenever we should declare gameover
				local allyTeams = GetAllyTeamList()
				local aliveAllyTeamCount = 0
				local winnerAllyTeams = {}
				for _,checkAllyTeamID in ipairs(allyTeams) do
					local checkTeamCount = allyTeamAliveTeamsCount[checkAllyTeamID]
					if checkTeamCount then
						if checkTeamCount ~= 0 then
							aliveAllyTeamCount = aliveAllyTeamCount +1
							winnerAllyTeams[1] = checkAllyTeamID
						end
					else
						-- wtf, this should not happend
					end
					if sharedDynamicAllianceVectory == 0 then
					--otherwise we have to cross check all the alliances
						for _,secondCheckAllyTeamID in ipairs(allyTeams) do

						end
					end
				end
				-- if there's no dynamic victory mode enabled, we can simply check if there's only one allyteam alive
				if sharedDynamicAllianceVectory == 0 and aliveAllyTeamCount == 1 then
					GameOver(winnerAllyTeams)
				end
			end
		else
			-- wtf, this should not happend
		end
	else
		-- wtf, this should not happend
	end
end

function gadget:Initialize()
	local allyTeams = GetAllyTeamList()
	for _,allyTeamID in ipairs(allyTeams) do
		local teamList = GetTeamList(allyTeamID)
		for _,teamID in ipairs(teamList) do
			teamToAllyTeam[teamID] = allyTeamID
		end
	end
end

function gadget:UnitCreated(unitID, unitDefID, unitTeamID)
	local teamUnitCount = teamsUnitCount[unitTeamID]
	if not teamUnitCount then
		teamUnitCount = 1
	else
		teamUnitCount = teamUnitCount + 1
	end
	teamsUnitCount[unitTeamID] = teamUnitCount
	local allyTeamID = teamToAllyTeam[unitTeamID]
	if allyTeamID then
		local allyTeamUnitCount = allyTeamAliveTeamsCount[allyTeamID]
		if not allyTeamUnitCount then
			allyTeamUnitCount = 1
		else
			allyTeamUnitCount = allyTeamUnitCount + 1
		end
		allyTeamAliveTeamsCount[allyTeamID] = allyTeamUnitCount
	else
		-- wtf, this should not happend
	end
end

function gadget:UnitGiven(unitID, unitDefID, unitTeamID)
	gadget:UnitCreated(unitID, unitDefID, unitTeamID)
end

function gadget:UnitCaptured(unitID, unitDefID, unitTeamID)
	gadget:UnitCreated(unitID, unitDefID, unitTeamID)
end

function gadget:UnitDestroyed(unitID, unitDefID, unitTeamID)
	local teamUnitCount = teamsUnitCount[unitTeamID]
	if teamUnitCount then
		teamUnitCount = teamUnitCount - 1
		teamsUnitCount[unitTeamID] = teamUnitCount
		if teamDieOnZeroUnits ~= 0 and teamUnitCount == 0 then
			-- no more units in the team, and option is enabled -> kill the team
			KillTeam( unitTeamID )
		end
	else
		-- wtf, this should not happend
	end
	local allyTeamID = teamToAllyTeam[unitTeamID]
	if allyTeamID then
		local allyTeamUnitCount = allyTeamAliveTeamsCount[allyTeamID]
		if allyTeamUnitCount then
			allyTeamUnitCount = allyTeamUnitCount - 1
			allyTeamAliveTeamsCount[allyTeamID] = allyTeamUnitCount
			if allyTeamDieOnZeroUnits ~= 0 and allyTeamUnitCount == 0 then
				-- no more units in the allyteam and the option is enabled -> kill all the teams in the allyteam
				local teamList = GetTeamList(allyTeamID)
				for _,teamID in ipairs(teamList) do
					KillTeam( teamID )
				end
			end
		else
			-- wtf, this should not happend
		end
	else
		-- wtf, this should not happend
	end
end
