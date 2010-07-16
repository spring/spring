--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    share_levels.lua
--  brief:   control resource sharing
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function gadget:GetInfo()
  return {
    name      = "ShareLevels",
    desc      = "Control resource sharing",
    author    = "trepan",
    date      = "Apr 22, 2007",
    license   = "GNU GPL, v2 or later",
    layer     = -5,
    enabled   = true  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local metalForce  = false
local metalLevel  = 0.5

local energyForce = false
local energyLevel = 0.5

-- for toggling
local prevMetal = false
local prevEnery = false

local cmdName = "sharelevels"
local help    = " <on|off|list|toggle|<<e|m|*> [<on|off|%>]>]>"
                .. ":  control automatic team resource sharing"


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



local function PrintHelp()
  Spring.Echo(help)
  return true
end


local function PrintState()
  if (not metalForce) then
    Spring.Echo('forced metal sharing is disabled')
  else
    Spring.Echo(string.format(
      'forced metal sharing level set to: %i%%',
      ((1 - metalLevel) * 100)
    ))
  end
  if (not energyForce) then
    Spring.Echo('forced energy sharing is disabled')
  else
    Spring.Echo(string.format(
      'forced energy sharing level set to: %i%%',
      ((1 - energyLevel) * 100)
    ))
  end
  return true
end


local function ToggleBoth()
  local hadPrev = (prevMetal or prevEnergy)
  if (metalForce or energyForce) then
    prevMetal  = metalForce
    prevEnergy = energyForce
    metalForce  = false
    energyForce = false
  elseif (hadPrev) then
    metalForce  = prevMetal
    energyForce = prevEnergy
  else
    metalForce  = true
    energyForce = true
  end
end


local function ChatControl(cmd, line, words, playerID)
  if (not AllowAction(playerID)) then
    PrintState()
    return true
  end
  
  local w1, w2 = words[1], words[2]
  
  if ((w1 == 'list') or (w1 == 'l')) then
    return PrintState()
  elseif ((w1 == 'toggle') or (w1 == 't')) then
    ToggleBoth()
    return PrintState()
  elseif (w1 == 'on') then
    metalForce  = true
    energyForce = true
    return PrintState()
  elseif (w1 == 'off') then
    metalForce  = false
    energyForce = false
    return PrintState()
  end

  if (not (w1 == 'e') and
      not (w1 == 'm') and
      not (w1 == '*')) then
    return PrintHelp()
  end

  if (w2 == nil) then
    if (w1 == 'm') then
      metalForce = not metalForce
      return PrintState()
    elseif (w1 == 'e') then
      energyForce = not energyForce
      return PrintState()
    else
      ToggleBoth()
      return PrintState()
    end
  end

  if ((w2 == 'on') or (w2 == 'off')) then
    local state = (w2 == 'on')
    if (w1 == 'm') then
      metalForce = state
      return PrintState()
    elseif (w1 == 'e') then
      energyForce = state
      return PrintState()
    else
      metalForce  = state
      energyForce = state
      return PrintState()
    end
    return PrintHelp()
  end

  local level = tonumber(w2)
  if (level == nil) then
    PrintHelp()
    Spring.Echo("bad sharing level: " .. w2)
    return true
  end
  level = (100 - level) * 0.01
  if (level < 0) then level = 0 end
  if (level > 1) then level = 1 end

  if ((w1 == 'm') or (w1 == '*')) then
    metalLevel = level
    metalForce = true
  end
  if ((w1 == 'e') or (w1 == '*')) then
    energyLevel = level
    energyForce = true
  end

  return PrintState()
end


--------------------------------------------------------------------------------

function gadget:Initialize()
  if (not gadgetHandler:IsSyncedCode()) then
    gadgetHandler:RemoveGadget()
    return
  end
  gadgetHandler:AddChatAction(cmdName, ChatControl, help)
  Script.AddActionFallback(cmdName .. ' ', help)
end


function gadget:Shutdown()
  gadgetHandler:RemoveChatAction(cmdName, ChatControl)
end


--------------------------------------------------------------------------------

local GetTeamList       = Spring.GetTeamList
local SetTeamShareLevel = Spring.SetTeamShareLevel


function gadget:GameFrame()
  if (not (metalForce or energyForce)) then
    return
  end
  for _,team in ipairs(GetTeamList()) do
    if (metalForce)  then SetTeamShareLevel(team, 'm', metalLevel)  end
    if (energyForce) then SetTeamShareLevel(team, 'e', energyLevel) end
  end
  return false
end


function gadget:AllowResourceLevel(teamID, type, level)
  if (type == 'm') then
    return not metalForce
  elseif (type == 'e') then
    return not energyForce
  end
  return true
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
