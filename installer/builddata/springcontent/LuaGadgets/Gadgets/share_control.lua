--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    share_control.lua
--  brief:   only allow sharing with allied teams
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function gadget:GetInfo()
  return {
    name      = "ShareControl",
    desc      = "Controls sharing of units and resources",
    author    = "trepan",
    date      = "Apr 22, 2007",
    license   = "GNU GPL, v2 or later",
    layer     = -5,
    enabled   = true  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local resShare  = true  -- allow sharing resources

local unitShare = true  -- allow sharing units

local resShareEnemy  = false  -- allow sharing resources with enemies

local unitShareEnemy = false  -- allow sharing units with enemies


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
  Spring.Echo('sharing units is '
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
  local count = #words

  if (count == 0) then
    unitShare = not unitShare
    resShare  = unitShare
  elseif (count == 1) then
    if ((words[1] == '0') or (words[1] == 'none')) then
      resShare  = false
      unitShare = false
      resShareEnemy  = false
      unitShareEnemy = false
    elseif ((words[1] == '1') or (words[1] == 'ally')) then
      resShare  = true
      unitShare = true
      resShareEnemy  = false
      unitShareEnemy = false
    elseif ((words[1] == '2') or (words[1] == 'full')) then
      resShare  = true
      unitShare = true
      resShareEnemy  = true
      unitShareEnemy = true
    elseif (words[1] == 'r') then
      resShare  = not resShare
    elseif (words[1] == 'u') then
      unitShare  = not unitShare
    elseif (words[1] == 'e') then
      resShareEnemy  = not resShareEnemy
      unitShareEnemy = not unitShareEnemy
    end
  elseif (count == 2) then
    if (words[1] == 'e') then
      if (words[2] == '0') then
        resShareEnemy  = false
        unitShareEnemy = false
      elseif (words[2] == '1') then
        resShareEnemy  = true
        unitShareEnemy = true
      end
    elseif (words[1] == 'r') then
      if (words[2] == '0') then
        resShare = false
      elseif (words[2] == '1') then
        resShare = true
      elseif (words[2] == 'e') then
        resShareEnemy = not resShareEnemy
      end
    elseif (words[1] == 'u') then
      if (words[2] == '0') then
        unitShare = false
      elseif (words[2] == '1') then
        unitShare = true
      elseif (words[2] == 'e') then
        unitShareEnemy = not unitShareEnemy
      end
    end
  elseif (count == 3) then
    if (words[1] == 'r') then
      if (words[2] == 'e') then
        if (words[3] == '0') then
          resShareEnemy = false
        elseif (words[3] == '1') then
          resShareEnemy = true
        end
      end
    elseif (words[1] == 'u') then
      if (words[2] == 'e') then
        if (words[3] == '0') then
          unitShareEnemy = false
        elseif (words[3] == '1') then
          unitShareEnemy = true
        end
      end
    end
  end

  PrintState()
  return true
end


function gadget:Initialize()
  if (not gadgetHandler:IsSyncedCode()) then
    gadgetHandler:RemoveGadget()
    return
  end
  local cmd, help
  
  cmd  = "sharectrl"
  local h = ''
  h = h..     ' [ "none" | "ally" | "full"]:  basic sharing modes\n'
  h = h..cmd..' <"u"|"r"> ["e"] [0|1]:  finer sharing control\n'
  h = h..'  u: unit sharing\n'
  h = h..'  r: resource sharing\n'
  h = h..'  e: enemy mode\n'
  
--  h = h..cmd..' [0|1]:  control unit and resource sharing\n'
--  h = h..cmd..' res [0|1]:  control resource sharing\n'
--  h = h..cmd..' unit [0|1]:  control unit sharing\n'
--  h = h..cmd..' enemy [0|1]:  control enemy unit and resource sharing\n'
--  h = h..cmd..' enemy res [0|1]:  control enemy resource sharing\n'
  help = h
  gadgetHandler:AddChatAction(cmd, ChatControl, help)
  Script.AddActionFallback(cmd .. ' ', help)
end


function gadget:Shutdown()
  gadgetHandler:RemoveChatAction("sharectrl", ChatControl)
end


function gadget:AllowResourceTransfer(oldTeam, newTeam, type, amount)
  if (resShare) then
    if (resShareEnemy or Spring.AreTeamsAllied(oldTeam, newTeam)) then
      return true
    else
      Spring.SendMessageToTeam(oldTeam, "Can not give resources to enemies")
    end
  else
    Spring.SendMessageToTeam(oldTeam, "Resource sharing has been disabled")
  end
  return false
end


local function AddRefusal(team, msg)
  local frameNum = Spring.GetGameFrame()
  local lastRefusal = lastRefusals[team]
  if ((not lastRefusal) or (lastRefusal ~= frameNum)) then
    lastRefusals[team] = frameNum
    Spring.SendMessageToTeam(team, msg)
  end
end


function gadget:AllowUnitTransfer(unitID, unitDefID, oldTeam, newTeam, capture)
  if (capture) then
    return true
  end

  if (Spring.IsCheatingEnabled()) then
    return true
  end

  if (not unitShare) then
    AddRefusal(oldTeam, "Unit sharing has been disabled")
    return false
  end
    
  if (unitShareEnemy or Spring.AreTeamsAllied(oldTeam, newTeam)) then
    return true
  end

  AddRefusal(oldTeam, "Can not give units to enemies")
  return false
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
