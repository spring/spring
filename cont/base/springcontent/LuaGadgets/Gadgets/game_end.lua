--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    game_end.lua
--  brief:   handles trigger conditions for game over
--  author:  Andrea Piras
--
--  Copyright (C) 2010-2013.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function gadget:GetInfo()
	return {
		name      = "Game End",
		desc      = "Handles team/allyteam deaths and declares gameover",
		author    = "Andrea Piras",
		date      = "June, 2013",
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
local teamDeathMode = modOptions.teamdeathmode or "allyzerounits"

-- sharedDynamicAllianceVictory is a C-like bool
local sharedDynamicAllianceVictory = tonumber(modOptions.shareddynamicalliancevictory) or 0

-- ignoreGaia is a C-like bool
local ignoreGaia = tonumber(modOptions.ignoregaiawinner) or 1

--playerQuit is a C-like bool
local playerQuitIsDead = tonumber(modOptions.playerquitisdead) or 1

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local KillTeam = Spring.KillTeam
local GetAllyTeamList = Spring.GetAllyTeamList
local GetTeamList = Spring.GetTeamList
local GetPlayerInfo = Spring.GetPlayerInfo
local GetPlayerList = Spring.GetPlayerList
local GetTeamInfo = Spring.GetTeamInfo
local GetTeamUnitCount = Spring.GetTeamUnitCount
local GetAIInfo = Spring.GetAIInfo
local GetTeamLuaAI = Spring.GetTeamLuaAI
local GameOver = Spring.GameOver
local AreTeamsAllied = Spring.AreTeamsAllied

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--allyTeamInfos structure:
-- allyTeamInfos = {
--	[allyTeamID] = {
--		teams = {
--			[teamID]= {
--				unitCount,
--				isGaia,
--				dead,
--				isAI,
--				isControlled,
--				players = {
--					[playerID] = isControlling
--				},
--			},
--		},
--		unitCount,
--		isGaia,
--		dead,
--	},
--}
local allyTeamInfos = {}
local teamToAllyTeam = {}
local playerIDtoAIs = {}
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------


function gadget:GameOver()
	-- remove ourself after successful game over
	gadgetHandler:RemoveGadget()
end

function gadget:Initialize()
	if teamDeathMode == "none" then
		gadgetHandler:RemoveGadget()
	end
	local gaiaTeamID = Spring.GetGaiaTeamID()
	-- at start, fill in the table of all alive allyteams
	for _,allyTeamID in ipairs(GetAllyTeamList()) do
		local allyTeamInfo = {}
		allyTeamInfo.unitCount = 0
		allyTeamInfo.teams = {}
		for _,teamID in ipairs(GetTeamList(allyTeamID)) do
			teamToAllyTeam[teamID] = allyTeamID
			local teamInfo = {}
			teamInfo.players = {}
			--is it engine ai?
			teamInfo.isAI = select(4,GetTeamInfo(teamID))
			teamInfo.hasLeader = select(2,GetTeamInfo(teamID)) >= 0
			if teamInfo.isAI then
				--store who hosts that engine ai
				local AIHostPlayerID = select(3,GetAIInfo(teamID))
				playerIDtoAIs[AIHostPlayerID] = playerIDtoAIs[AIHostPlayerID] or {}
				playerIDtoAIs[AIHostPlayerID][teamID] = allyTeamID
			end
			--is luaai
			if GetTeamLuaAI(teamID) ~= "" then
				teamInfo.isAI = true
				teamInfo.isControlled = true
			end
			--is gaia
			if teamID == gaiaTeamID then
				allyTeamInfo.isGaia = true
				teamInfo.isGaia = true
				teamInfo.isControlled = true
			end
			teamInfo.unitCount = GetTeamUnitCount(teamID)
			allyTeamInfo.unitCount = allyTeamInfo.unitCount + teamInfo.unitCount 
			allyTeamInfo.teams[teamID] = teamInfo
		end
		allyTeamInfos[allyTeamID] = allyTeamInfo
	end
	for _,playerID in ipairs(GetPlayerList()) do
		gadget:PlayerChanged(playerID)
	end
end

local function IsCandidateWinner(allyTeamID)
	return not allyTeamInfos[allyTeamID].dead and (ignoreGaia == 0 or not allyTeamInfos[allyTeamID].isGaia)
end

local function CheckSingleAllyVictoryEnd()
	local winnerCount = 0
	local candidateWinners = {}
	-- find the last remaining allyteam
	for allyTeamID in pairs(allyTeamInfos) do
		if IsCandidateWinner(allyTeamID) then
			winnerCount = winnerCount + 1
			candidateWinners[#candidateWinners+1] = allyTeamID
		end
	end
	if #candidateWinners > 1 then
		return false
	end
	return candidateWinners
end

local function AreAllyTeamsDoubleAllied(firstAllyTeamID,  secondAllyTeamID)
	-- we need to check for both directions of alliance
	for teamA in pairs(allyTeamInfos[firstAllyTeamID].teams) do
		for teamB in pairs(allyTeamInfos[secondAllyTeamID].teams) do
			if not AreTeamsAllied(teamA, teamB) or not AreTeamsAllied(teamB, teamA) then
				return false
			end 
		end
	end
	return true
end

local function CheckSharedAllyVictoryEnd()
	-- we have to cross check all the alliances
	local candidateWinners = {}
	local winnerCountSquared = 0
	local aliveCount = 0
	for allyTeamA in pairs(allyTeamInfos) do
		if IsCandidateWinner(allyTeamA) then
			aliveCount = aliveCount + 1
			for allyTeamB in pairs(allyTeamInfos) do
				if IsCandidateWinner(allyTeamB) and AreAllyTeamsDoubleAllied(allyTeamA, allyTeamB) then
					-- store both check directions
					-- since we're gonna check if we're allied against ourself, only secondAllyTeamID needs to be stored
					candidateWinners[allyTeamB] =  true
					winnerCountSquared = winnerCountSquared + 1
				end
			end
		end
	end

	if aliveCount*aliveCount ~= winnerCountSquared then
		return false
	end
	
	-- all the allyteams alive are bidirectionally allied against eachother, they are all winners
	local winnersCorrectFormat = {}
	for winner in pairs(candidateWinners) do
		winnersCorrectFormat[#winnersCorrectFormat+1] = winner
	end
	return winnersCorrectFormat

end


local function UpdateAllyTeamIsDead(allyTeamID)
	local allyTeamInfo = allyTeamInfos[allyTeamID]
	local dead = true
	for teamID,teamInfo in pairs(allyTeamInfo.teams) do
		if playerQuitIsDead == 0 then
			dead = dead and (teamInfo.dead or not teamInfo.hasLeader )
		else
			dead = dead and (teamInfo.dead or not teamInfo.isControlled )
		end
	end
	allyTeamInfos[allyTeamID].dead = dead
end

function gadget:GameFrame()
	for _,playerID in ipairs(GetPlayerList()) do
		gadget:PlayerChanged(playerID)
	end
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


function gadget:PlayerChanged(playerID)
	local _,active,spectator,teamID = GetPlayerInfo(playerID) 
	local allyTeamID = teamToAllyTeam[teamID] 
	local teamInfo = allyTeamInfos[allyTeamID].teams[teamID]
	teamInfo.players[playerID] = active and not spectator
	teamInfo.hasLeader = select(2,GetTeamInfo(teamID)) >= 0
	if not teamInfo.hasLeader and not teamInfo.dead then
		KillTeam(teamID)
	end
	if not teamInfo.isAI then
		--if team isn't ai controlled, then we need to check if we have attached players
		teamInfo.isControlled = false
		for _,isControlling in pairs(teamInfo.players) do
			if isControlling then
				teamInfo.isControlled = true
				break
			end
		end
	end
	--if player is an AI controller, then mark all hosted AIs as uncontrolled
	local AIHostList = playerIDtoAIs[playerID] or {}
	for AITeam, AIAllyTeam in pairs(AIHostList) do
		allyTeamInfos[AIAllyTeam].teams[AITeam].isControlled = active
	end
	allyTeamInfos[allyTeamID].teams[teamID] = teamInfo
	UpdateAllyTeamIsDead(allyTeamID)
end



function gadget:TeamDied(teamID)
	local allyTeamID = teamToAllyTeam[teamID]
	local allyTeamInfo = allyTeamInfos[allyTeamID]
	allyTeamInfo.teams[teamID].dead = true
	allyTeamInfos[allyTeamID] = allyTeamInfo
	UpdateAllyTeamIsDead(allyTeamID)
end

function gadget:UnitCreated(unitID, unitDefID, unitTeamID)
	local allyTeamID = teamToAllyTeam[unitTeamID]
	local allyTeamInfo = allyTeamInfos[allyTeamID]
	allyTeamInfo.teams[unitTeamID].unitCount = allyTeamInfo.teams[unitTeamID].unitCount + 1
	allyTeamInfo.unitCount = allyTeamInfo.unitCount + 1
	allyTeamInfos[allyTeamID] = allyTeamInfo
end

gadget.UnitGiven = gadget.UnitCreated
gadget.UnitCaptured = gadget.UnitCreated

function gadget:UnitDestroyed(unitID, unitDefID, unitTeamID)
	local allyTeamID = teamToAllyTeam[unitTeamID]
	local allyTeamInfo = allyTeamInfos[allyTeamID]
	local teamUnitCount = allyTeamInfo.teams[unitTeamID].unitCount -1
	local allyTeamUnitCount = allyTeamInfo.unitCount - 1
	allyTeamInfo.teams[unitTeamID].unitCount = teamUnitCount
	allyTeamInfo.unitCount = allyTeamUnitCount
	allyTeamInfos[allyTeamID] = allyTeamInfo
	if allyTeamInfo.isGaia and ignoreGaia == 1 then
		return
	end
	if teamDeathMode == "teamzerounits" and teamUnitCount == 0 then
		KillTeam(unitTeamID)
	elseif teamDeathMode == "allyzerounits" and allyTeamUnitCount == 0 then
		for teamID in pairs(allyTeamInfo.teams) do
			KillTeam(teamID)
		end
	end
end

gadget.UnitTaken = gadget.UnitDestroyed
