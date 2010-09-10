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
-- ignoreGaia is a C-like bool
local ignoreGaia = tonumber(modOptions.ignoregaiawinner) or 1

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

local gaiaAllyTeamID
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

local function IsCandidateWinner(allyTeamID)
	local allyTeamUnitCount = allyTeamAliveTeamsCount[allyTeamID]
	local gaiaCheck = ignoreGaia == 0 or allyTeamID ~= gaiaAllyTeamID
	return allyTeamUnitCount and allyTeamUnitCount ~= 0 and gaiaCheck
end

local function CheckSingleAllyVictoryEnd()
	if aliveAllyTeamCount ~= 1 then
		return false
	end
	-- find the last remaining allyteam
	for _,candidateWinner in ipairs(allyTeams) do
		if IsCandidateWinner(candidateWinner) then
			return {candidateWinner}
		end
	end
	return false
end

local function AreAllyTeamsDoubleAllied( firstAllyTeamID,  secondAllyTeamID )
	-- we need to check for both directions of alliance
	return AreTeamsAllied( firstAllyTeamID,  secondAllyTeamID ) and AreTeamsAllied( secondAllyTeamID, firstAllyTeamID )
end

local function CheckSharedAllyVictoryEnd()
	-- we have to cross check all the alliances
	local candidateWinners = {}
	local winnerCountSquared = 0
	for _,firstAllyTeamID in ipairs(allyTeams) do
		if IsCandidateWinner(firstAllyTeamID) then
			for _,secondAllyTeamID in ipairs(allyTeams) do
				if IsCandidateWinner(secondAllyTeamID) and AreAllyTeamsDoubleAllied( firstAllyTeamID,  secondAllyTeamID ) then
					-- store both check directions
					-- since we're gonna check if we're allied against ourself, only secondAllyTeamID needs to be stored
					candidateWinners[secondAllyTeamID] =  true
					winnerCountSquared = winnerCountSquared +1
				end
			end
		end
	end
	if winnerCountSquared == (aliveAllyTeamCount*aliveAllyTeamCount) then
		-- all the allyteams alive are bidirectionally allied against eachother, they are all winners
		local winnersCorrectFormat = {}
		for winner in pairs(candidateWinners) do
			winnersCorrectFormat[#winnersCorrectFormat+1] = winner
		end
		return winnersCorrectFormat
	end
	-- couldn't find any winner
	return false
end

local function CheckGameOver()
	local winners
	if sharedDynamicAllianceVictory == 0 then
		winners = CheckSingleAllyVictoryEnd()
	else
		winners = CheckSharedAllyVictoryEnd()
	end
	if winners then
		GameOver(winners)
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
	if sharedDynamicAllianceVictory == 0 then
		-- if dynamic alliance is off, there's no point of checking gameover every slowupdate, since the hook for unit died is sufficient
		gadgetHandler:RemoveCallIn( "GameFrame", self )
	end
	local allyTeamCount = 0
	-- at start, fill in the table of all alive allyteams
	for _,allyTeamID in ipairs(allyTeams) do
		local teamList = GetTeamList(allyTeamID)
		local teamCount = 0
		for _,teamID in ipairs(teamList) do
			teamToAllyTeam[teamID] = allyTeamID
			if teamID == gaiaTeamID then
				if ignoreGaia == 0 then
					teamCount = teamCount + 1
				end
				gaiaAllyTeamID = allyTeamID
			else
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
	if unitTeamID == gaiaTeamID and ignoreGaia ~= 0 then
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

function gadget:UnitTaken(unitID, unitDefID, unitTeamID)
	gadget:UnitDestroyed(unitID, unitDefID, unitTeamID)
end
