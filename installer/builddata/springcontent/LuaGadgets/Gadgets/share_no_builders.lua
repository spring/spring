--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    share_no_builders.lua
--  brief:   disallow sharing of builders and factories
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function gadget:GetInfo()
  return {
    name      = "ShareNoBuilders",
    desc      = "Disallow sharing of builders and factories",
    author    = "trepan",
    date      = "Apr 22, 2007",
    license   = "GNU GPL, v2 or later",
    layer     = -5,
    enabled   = true  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local enabled = true

--
--  A team indexed list of the last frames that a unit transfer was blocked.
--  This helps to avoid filling up the console with warnings when a user tries
--  to share multiple units.
--
--
local lastRefusals = {}


--------------------------------------------------------------------------------

local function AllowAction(playerID)
  if (playerID ~= 0) then
    Spring.SendMessageToPlayer(playerID, "Must be the host player")
    return false
  end
  if (not Spring.IsCheatingEnabled()) then
    Spring.SendMessageToPlayer(playerID, "Cheating must be enabled")
    return false
  end
  return true
end


local function PrintState()
  Spring.Echo('sharing buildersunits is '
              .. (unitShare and 'enabled' or 'disabled'))
  Spring.Echo('sharing resources is '
              .. (resShare and 'enabled' or 'disabled'))
  Spring.Echo('sharing units with enemies is '
              .. (unitShare and unitShareEnemy and 'enabled' or 'disabled'))
  Spring.Echo('sharing resources with enemies is '
              .. (resShare and resShareEnemy and 'enabled' or 'disabled'))
  return true
end


local function ChatControl(cmd, line, words, playerID)
  if (not AllowAction(playerID)) then
    PrintState()
    return true
  end
--FIXME  if (words[1] == 
  PrintState()
  return true
end


function gadget:Initialize()
  if (not gadgetHandler:IsSyncedCode()) then
    gadgetHandler:RemoveGadget()
    return
  end
  local cmd, help
  
  cmd  = "sharenobuilders"
  local h = ''
  h = h..     ' [ "none" | "ally" | "full"]:  basic sharing modes\n'
  h = h..cmd..' <"u"|"r"> ["e"] [0|1]:  finer sharing control\n'
  h = h..'  u: unit sharing\n'
  h = h..'  r: resource sharing\n'
  h = h..'  e: enemy mode\n'
  
  help = h
  gadgetHandler:AddChatAction(cmd, ChatControl, help)
  Script.AddActionFallback(cmd .. ' ', help)
end


function gadget:Shutdown()
  gadgetHandler:RemoveChatAction("sharenobuilders", ChatControl)
end


local function AddRefusal(team, msg)
  local frameNum = Spring.GetGameFrame()
  local lastRefusal = lastRefusals[team]
  if ((not lastRefusal) or (lastRefusal ~= frameNum)) then
    lastRefusals[team] = frameNum
    Spring.SendMessageToTeam(team, msg)
  end
end


local function TeamHasBuilder(teamID)
end


local function TeamCanBeTaken(teamID)
  local players = Spring.GetPlayerList(teamID, true)
  for _, playerID in ipairs(players) do
    local name, active, spec = Spring.GetPlayerInfo(playerID)
    if (active or (not spec)) then
      return false
    end
  end
  return true
end


local function TeamHasNoBuilders(teamID)
  local units = Spring.GetTeamUnits(teamID)
  for _, unitID in ipairs(units) do
    local ud = UnitDefs[Spring.GetUnitDefID(unitID)]
    if (ud and ud.builder) then
      return false
    end
  end
  return true
end


function gadget:AllowUnitTransfer(unitID, unitDefID, oldTeam, newTeam, capture)
  if (capture or (not enabled)) then
    return true
  end

  if (Spring.IsCheatingEnabled()) then
    return true
  end

  local ud = UnitDefs[unitDefID]
  if ((not ud) or (not ud.builder)) then
    return true -- not a builder
  end

  if (TeamCanBeTaken(oldTeam)) then
    return true -- .take always succeeds
  end

  if (ud.isFactory and (ud.techLevel == 1)) then
    if (TeamHasNoBuilders(newTeam)) then
      return true -- let the damned have a factory
    end
  end

  AddRefusal(oldTeam, "Can not share builders unless the receiver has none")
  return false
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
