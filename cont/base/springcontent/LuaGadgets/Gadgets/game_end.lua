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
local gaiaTeamID = Spring.GetGaiaTeamID()
local KillTeam = Spring.KillTeam
local GetAllyTeamList = Spring.GetAllyTeamList
local GetTeamList = Spring.GetTeamList
local GameOver = Spring.GameOver
local AreTeamsAllied = Spring.AreTeamsAllied

local teamDieOnZeroUnits = modOptions.teamDieOnZeroUnits or 0
local allyTeamDieOnZeroUnits = modOptions.allyTeamDieOnZeroUnits or 1
local sharedDynamicAllianceVictory = modOptions.sharedDynamicAllianceVictory or 0

local teamsUnitCount = {}
local allyTeamUnitCount = {}
local allyTeamAliveTeamsCount = {}
local teamToAllyTeam = {}
local aliveAllyTeamCount = 0

local function CheckGameOver()
	local allyTeams = GetAllyTeamList()

	if sharedDynamicAllianceVictory == 0 then
		if aliveAllyTeamCount == 1 then
		-- find the last remaining allyteam
			for _,firstAllyTeamID in ipairs(allyTeams) do
				local firstTeamCount = allyTeamAliveTeamsCount[firstAllyTeamID]
				if firstTeamCount and firstAllyTeamID ~= gaiaTeamID then
					if firstTeamCount ~= 0 then
						GameOver({firstTeamCount})
						return
					end
				end
			end
		end
	else
		-- otherwise we have to cross check all the alliances
		for _,firstAllyTeamID in ipairs(allyTeams) do
			local firstTeamCount = allyTeamAliveTeamsCount[firstAllyTeamID]
			if firstTeamCount and firstAllyTeamID ~= gaiaTeamID then
				if firstTeamCount ~= 0 then
					local crossAllianceCount = 0
					for _,secondAllyTeamID in ipairs(allyTeams) do
						local secondTeamCount = allyTeamAliveTeamsCount[secondAllyTeamID]
						if secondTeamCount and secondAllyTeamID ~= gaiaTeamID then
							if secondTeamCount ~= 0 then
								-- we need to check for both directions of alliance
								if AreTeamsAllied( firstAllyTeamID,  secondAllyTeamID ) and AreTeamsAllied( secondAllyTeamID, firstAllyTeamID ) then
									crossAllianceCount = crossAllianceCount + 1
								end
							end
						end
					end
					if crossAllianceCount == aliveAllyTeamCount then
						-- only allyteams alive are all bidirectionally allied, they are all winners
						GameOver(allyTeams)
						return
					end
				end
			end
		end
	end

end

function gadget:GameFrame(frame)
	if (frame%32) == 0 then -- only do a check in slowupdate
		CheckGameOver()
	end
end

function gadget:TeamDied(teamID)
	local allyTeamID = teamToAllyTeam[teamID]
	if allyTeamID then
		local aliveTeamCount = allyTeamAliveTeamsCount[allyTeamID]
		if aliveTeamCount then
			aliveTeamCount = aliveTeamCount -1
			allyTeamAliveTeamsCount[allyTeamID] = aliveTeamCount
			if aliveTeamCount == 0 then -- one allyteam just died, check the others whenever we should declare gameover
				aliveAllyTeamCount = aliveAllyTeamCount - 1
				CheckGameOver()
			end
		end
	end
end

function gadget:Initialize()
	local allyTeamCount = 0
	local allyTeams = GetAllyTeamList()
	for _,allyTeamID in ipairs(allyTeams) do
		local teamList = GetTeamList(allyTeamID)
		local teamCount = 0
		for _,teamID in ipairs(teamList) do
			if teamID ~= gaiaTeamID then
				teamToAllyTeam[teamID] = allyTeamID
				teamCount = teamCount + 1
			end
		end
		allyTeamAliveTeamsCount[allyTeamID] = teamCount
		if teamCount > 0 then
			 allyTeamCount = allyTeamCount +1
		end
	end
	aliveAllyTeamCount = allyTeamCount
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
		local unitCount = allyTeamUnitCount[allyTeamID]
		if not unitCount then
			unitCount = 1
		else
			unitCount = unitCount + 1
		end
		allyTeamUnitCount[allyTeamID] = unitCount
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
	end
	local allyTeamID = teamToAllyTeam[unitTeamID]
	if allyTeamID then
		local unitCount = allyTeamUnitCount[allyTeamID]
		if unitCount then
			unitCount = unitCount - 1
			allyTeamUnitCount[allyTeamID] = unitCount
			if allyTeamDieOnZeroUnits ~= 0 and unitCount == 0 then
				-- no more units in the allyteam and the option is enabled -> kill all the teams in the allyteam
				local teamList = GetTeamList(allyTeamID)
				for _,teamID in ipairs(teamList) do
					KillTeam( teamID )
				end
			end
		end
	end
end
