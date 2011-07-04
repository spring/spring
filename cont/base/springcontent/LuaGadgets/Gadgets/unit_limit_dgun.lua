--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    unit_dgun_limit.lua
--  brief:   Re-implements limit dgun in Lua
--  author:  Andrea Piras
--
--  Copyright (C) 2010.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function gadget:GetInfo()
  return {
    name      = "Limit Dgun",
    desc      = "Re-implements limit dgun in Lua",
    author    = "Andrea Piras",
    date      = "August, 2010",
    license   = "GNU GPL, v2 or later",
    layer     = 0,
    enabled   = true  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local enabled = tonumber(Spring.GetModOptions().limitdgun) or 0
if (enabled == 0) then
  return false
end

local dgunMapFraction = 3 -- use the modoption to define it instead?
local dgunRadiusSquared = (Game.mapSizeX*Game.mapSizeZ) / (dgunMapFraction*dgunMapFraction)
local dgunRadius = math.sqrt(dgunRadiusSquared)

if (not gadgetHandler:IsSyncedCode()) then --begin unsynced section

local glColor = gl.Color
local glDrawGroundCircle = gl.DrawGroundCircle
local glPushMatrix = gl.PushMatrix
local glPopMatrix = gl.PopMatrix
local glDeleteList = gl.DeleteList
local glCreateList = gl.CreateList
local glCallList = gl.CallList

local GetSpectatingState = Spring.GetSpectatingState
local GetTeamStartPosition = Spring.GetTeamStartPosition
local GetTeamList = Spring.GetTeamList
local GetTeamStartPosition = Spring.GetTeamStartPosition
local AreTeamsAllied = Spring.AreTeamsAllied
local GetGaiaTeamID = Spring.GetGaiaTeamID
local GetMyTeamID = Spring.GetMyTeamID
local GetTeamInfo = Spring.GetTeamInfo

local allyTeamVec = Spring.GetAllyTeamList()
local myTeamID = GetMyTeamID()
local gaiaTeamID = GetGaiaTeamID()
local circlesList

function ReGenerateDisplayList()
	if circlesList then
		glDeleteList(circlesList)
	end
	circlesList = glCreateList( function()
		for allyIndex,allyTeamID in ipairs(allyTeamVec) do
			local teamList = GetTeamList(allyTeamID)
			for _,teamID in ipairs(teamList) do
				local _,_,isDead = GetTeamInfo(teamID)
				if not isDead and teamID ~= gaiaTeamID then
					if teamID == myTeamID then
						glColor(0.0, 0.0, 1.0, 0.6) -- use blue circles for your range
					elseif AreTeamsAllied( teamID, myTeamID ) == true then -- order matters! this tells if he's allied with us, not vice-versa
						glColor(1.0, 1.0, 1.0, 0.6) -- use white circles for ally ranges
					else
						glColor(1.0, 1.0, 0.0, 0.6) -- use yellow circles for enemy ranges
					end
					local teamX,teamY,teamZ = GetTeamStartPosition(teamID)
					if teamX and teamY and teamZ then
						glDrawGroundCircle(teamX,teamY,teamZ, dgunRadius, 40)
					end
				end
			end
		end
	end)
end

function gadget:PlayerChanged() -- regenerate the display list, update even if it's not for ourself, so we can rid of unnecessary team circles
	ReGenerateDisplayList()
end

function gadget:Initialize()
	ReGenerateDisplayList()
end

function gadget:Shutdown()
	glDeleteList(circlesList)
end

function gadget:DrawWorld() -- paint dgun limit preview range circle
	if circlesList then
		glPushMatrix()
			glCallList(circlesList)
		glPopMatrix()
	end
end

else -- begin synced section

local GetTeamStartPosition = Spring.GetTeamStartPosition
local GetUnitPosition = Spring.GetUnitPosition
local CMD_MANUALFIRE = CMD.MANUALFIRE

local teamStartPosVec = {} -- cache for team starting pos

-- squared distance between 2 points in 3D cartesian space
function SqDistance(one, two)
	dx = one[1] - two[1]
	dy = one[2] - two[2]
	dz = one[3] - two[3]
	return dx*dx + dy*dy + dz*dz
end

function gadget:AllowCommand(unitID, unitDefID, teamID, cmdID, cmdParams, cmdOptions, cmdTag, synced)
	if cmdID ~= CMD_MANUALFIRE then -- pass all non-dgun commands
		return true
	end

	if not teamStartPosVec[teamID] then -- build cache if not available
		local teamX,teamY,teamZ = GetTeamStartPosition(teamID)
		if teamX and teamY and teamZ then
			teamStartPosVec[teamID] = {teamX,teamY,teamZ}
		end
	end
	local teamStartPos = teamStartPosVec[teamID]
	if not teamStartPos then -- wtf
		return true
	end

	local unitX, unitY, unitZ = GetUnitPosition(unitID)
	if not unitX or not unitY or not unitZ then -- wtf
		return true
	end
	local unitPos = {unitX,unitY,unitZ}

	-- restrict the command to the allowed area
	if SqDistance(teamStartPos,unitPos) > dgunRadiusSquared then -- unit outside of allowed range, block the command
		return false
	end

	return true
end

end -- end synced section
