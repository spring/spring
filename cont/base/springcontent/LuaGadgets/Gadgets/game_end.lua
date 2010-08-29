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
-- teamDeathMode possible values: "none", "teamzerounits" , "allyzerounits"
local teamDeathMode = modOptions.teamdeathmode or "teamzerounits"
-- sharedDynamicAllianceVictory is a C-like bool
local sharedDynamicAllianceVictory = tonumber(modOptions.shareddynamicalliancevictory) or 0

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local gaiaTeamID = Spring.GetGaiaTeamID()
local KillTeam = Spring.KillTeam
local GetAllyTeamList = Spring.GetAllyTeamList
local GetTeamList = Spring.GetTeamList
local GameOver = Spring.GameOver
local AreTeamsAllied = Spring.AreTeamsAllied

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local allyTeams = GetAllyTeamList()
local teamsUnitCount = {}
local allyTeamUnitCount = {}
local allyTeamAliveTeamsCount = {}
local teamToAllyTeam = {}
local aliveAllyTeamCount = 0

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------


function gadget:GameOver()
	-- remove ourself after successful game over
	gadgetHandler:RemoveGadget()
end

local function CheckGameOver()
	if sharedDynamicAllianceVictory == 0 then
		if aliveAllyTeamCount == 1 then
		-- find the last remaining allyteam
			for _,firstAllyTeamID in ipairs(allyTeams) do
				local firstTeamCount = allyTeamAliveTeamsCount[firstAllyTeamID]
				if firstTeamCount and firstAllyTeamID ~= gaiaTeamID then
					if firstTeamCount ~= 0 then
						GameOver({1=firstAllyTeamID})
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
					local winners = {}
					local winnerCount = 0
					for _,secondAllyTeamID in ipairs(allyTeams) do
						local secondTeamCount = allyTeamAliveTeamsCount[secondAllyTeamID]
						if secondTeamCount and secondAllyTeamID ~= gaiaTeamID then
							if secondTeamCount ~= 0 then
								-- we need to check for both directions of alliance
								if AreTeamsAllied( firstAllyTeamID,  secondAllyTeamID ) and AreTeamsAllied( secondAllyTeamID, firstAllyTeamID ) then
									-- store both check directions
									-- since we're gonna check if we're allied against ourself, only secondAllyTeamID needs to be stored
									winners[secondAllyTeamID] =  true
									winnerCount = winnerCount +1
								end
							end
						end
					end
					if winnerCount == aliveAllyTeamCount then
						-- only allyteams alive are all bidirectionally allied, they are all winners
						local winnersCorrectFormat = {}
						for winner in pairs(winners) do
							winnersCorrectFormat[#winnersCorrectFormat+1] = winner
						end
						GameOver(winnersCorrectFormat)
						return
					end
				end
			end
		end
	end

end

function gadget:GameFrame(frame)
	if (frame%16) == 0 then -- only do a check in slowupdate
		CheckGameOver()
	end
end

function gadget:TeamDied(teamID)
	local allyTeamID = teamToAllyTeam[teamID]
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

function gadget:Initialize()
	if teamDeathMode == "none" then
		-- all our checks are useless if teams cannot die
		gadgetHandler:RemoveGadget()
	end
	local allyTeamCount = 0
	for _,allyTeamID in ipairs(allyTeams) do
		local teamList = GetTeamList(allyTeamID)
		local teamCount = 0
		for _,teamID in ipairs(teamList) do
			teamToAllyTeam[teamID] = allyTeamID
			if teamID ~= gaiaTeamID then
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
	local teamUnitCount = teamsUnitCount[unitTeamID] or 0
	teamUnitCount = teamUnitCount + 1
	teamsUnitCount[unitTeamID] = teamUnitCount
	local allyTeamID = teamToAllyTeam[unitTeamID]
	local unitCount = allyTeamUnitCount[allyTeamID] or 0
	unitCount = unitCount + 1
	allyTeamUnitCount[allyTeamID] = unitCount
end

function gadget:UnitGiven(unitID, unitDefID, unitTeamID)
	gadget:UnitCreated(unitID, unitDefID, unitTeamID)
end

function gadget:UnitCaptured(unitID, unitDefID, unitTeamID)
	gadget:UnitCreated(unitID, unitDefID, unitTeamID)
end

function gadget:UnitDestroyed(unitID, unitDefID, unitTeamID)
	if unitTeamID == gaiaTeamID then
		-- skip gaia
		return
	end
	local teamUnitCount = teamsUnitCount[unitTeamID]
	if teamUnitCount then
		teamUnitCount = teamUnitCount - 1
		teamsUnitCount[unitTeamID] = teamUnitCount
		if teamDeathMode == "teamzerounits" and teamUnitCount == 0 then
			-- no more units in the team, and option is enabled -> kill the team
			KillTeam( unitTeamID )
		end
	end
	local allyTeamID = teamToAllyTeam[unitTeamID]
	local unitCount = allyTeamUnitCount[allyTeamID]
	if unitCount then
		unitCount = unitCount - 1
		allyTeamUnitCount[allyTeamID] = unitCount
		if teamDeathMode == "allyzerounits" and unitCount == 0 then
			-- no more units in the allyteam and the option is enabled -> kill all the teams in the allyteam
			local teamList = GetTeamList(allyTeamID)
			for _,teamID in ipairs(teamList) do
				KillTeam( teamID )
			end
		end
	end
end
