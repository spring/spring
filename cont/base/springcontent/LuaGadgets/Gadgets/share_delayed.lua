--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    share_delayed.lua
--  brief:   delay unit sharing
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function gadget:GetInfo()
  return {
    name      = "SharingDelayed",
    desc      = "delayed unit sharing",
    author    = "trepan",
    date      = "Apr 22, 2007",
    license   = "GNU GPL, v2 or later",
    layer     = -4,
    enabled   = true  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  FIXME: (TODO)
--  - Delayed resource sharing
--  - Visual indicators for units queued to be shared
--  - Update unit share times for fallen comrades
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

-- Only active in comm-ends games

local gameMode = Game.gameMode or 4

if (gameMode ~= 1) then
  return false
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Proposed Command ID Ranges:
--
--    all negative:  Engine (build commands)
--       0 -   999:  Engine
--    1000 -  9999:  Group AI
--   10000 - 19999:  LuaUI
--   20000 - 29999:  LuaCob
--   30000 - 39999:  LuaRules
--

local CMD_CANCEL_SHARE = 33999


--------------------------------------------------------------------------------
--  COMMON
--------------------------------------------------------------------------------
if (gadgetHandler:IsSyncedCode()) then
--------------------------------------------------------------------------------
--  SYNCED
--------------------------------------------------------------------------------

local teams  = {}  --  teamID = { unitID = shareInfo }
local shares = {}  --  unitID = oldTeam
local frames = {}  --  unitID = framee

local enabled = true

local minDelay  = 1    -- minimum delay between shares
local costScale = 0.1  -- add extra cycles based on unit costs

local cancelShareCmdDesc = {
  id   = CMD_CANCEL_SHARE,
  type = CMDTYPE.ICON,
  name = '\255\255\100\100NoShare',
  tooltip = 'Cancel the unit transfer',
  action = 'cancel_share',
}


--------------------------------------------------------------------------------
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


local function ChatControl(cmd, line, words, playerID)
  if (not AllowAction(playerID)) then
    Spring.Echo('delayed sharing is ' .. (enabled and 'enabled' or 'disabled'))
    return true
  end
  if (#words == 0) then
    enabled = not enabled
  else
    enabled = (words[1] == '1')
  end
  Spring.Echo('delayed sharing is ' .. (enabled and 'enabled' or 'disabled'))
  return true
end


local function StopShare(cmd, line, words, playerID)
  local _,_,_,teamID = Spring.GetPlayerInfo(playerID)
  local team = teamID and teams[teamID] or nil
  if (team) then
    for _,data in pairs(team) do
      shares[data.unitID] = nil
      frames[data.unitID] = nil
    end
    teams[teamID] = nil
    Spring.Echo('cancelled remaining unit transfers')
  else
    Spring.Echo('there are no unit transfers to cancel')
  end
  return true
end


--------------------------------------------------------------------------------

function gadget:Initialize()
  gadgetHandler:RegisterCMDID(CMD_CANCEL_SHARE)
  _G.shareFrames = frames

  local cmd, help

  cmd  = "sharedelay"
  help = " [0|1]:  delayed unit sharing, useful for comm ends games"
  gadgetHandler:AddChatAction(cmd, ChatControl, help)
  Script.AddActionFallback(cmd .. ' ', help)

  cmd  = "stopshare"
  help = ":  cancel all queued unit transfers for your team"
  gadgetHandler:AddChatAction(cmd,  StopShare, help)
  Script.AddActionFallback(cmd, help)
end


function gadget:Shutdown()
  gadgetHandler:RemoveChatAction("sharedelay")
  Script.RemoveActionFallback("sharedelay")

  gadgetHandler:RemoveChatAction("stopshare")
  Script.RemoveActionFallback("stopshare")

  for _,unitID in ipairs(Spring.GetAllUnits()) do
    local cmdDescID = Spring.FindUnitCmdDesc(unitID, CMD_CANCEL_SHARE)
    if (cmdDescID) then
      Spring.RemoveUnitCmdDesc(unitID, cmdDescID)
    end
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local insert = table.insert
local remove = table.remove


local function InsertShare(unitID, oldTeam, newTeam, delay)
  if (shares[unitID]) then
    return  -- already active
  end

  local shareInfo = {
    unitID  = unitID,
    oldTeam = oldTeam,
    newTeam = newTeam,
    delay   = delay,
  }
  local team = teams[oldTeam]

  if (team) then
    print(team[#team].frame, delay)
    shareInfo.frame = team[#team].frame + delay
  else
    team = {}
    teams[oldTeam] = team
    shareInfo.frame = Spring.GetGameFrame() + delay
  end

  insert(team, shareInfo)
  shares[unitID] = oldTeam
  frames[unitID] = shareInfo.frame

  Spring.InsertUnitCmdDesc(unitID, 1, cancelShareCmdDesc)
end


--------------------------------------------------------------------------------

local function RecalcDelays(team)
  for i = 2, #team do
    team[i].frame = team[i - 1].frame + team[i].delay
    frames[team[i].unitID] = team[i].frame
  end
end


local function RemoveShare(unitID)
  local oldTeam = shares[unitID]
  shares[unitID] = nil
  frames[unitID] = nil

  local cmdDescID = Spring.FindUnitCmdDesc(unitID, CMD_CANCEL_SHARE)
  if (cmdDescID) then
    Spring.RemoveUnitCmdDesc(unitID, cmdDescID)
  end

  local team = teams[oldTeam]
  if ((oldTeam == nil) or (team == nil)) then
    return  -- not active
  end

  local index
  for i, shareInfo in ipairs(team) do
    if (shareInfo.unitID == unitID) then
      index = i
      break
    end
  end
  if (index == nil) then
    return  -- something is amiss
  end

  local shareInfo = team[index]
  remove(team, index)

  if (#team <= 0) then
    teams[oldTeam] = nil
  else
    if (index ~= 1) then
      RecalcDelays(team)
    else
      local nowFrame = Spring.GetGameFrame()
      if (shareInfo.frame > nowFrame) then
        local front = team[1]
        front.frame = nowFrame + front.delay
        frames[front.unitID] = front.frame
        RecalcDelays(team)
      end
    end
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function gadget:AllowUnitTransfer(unitID, unitDefID, oldTeam, newTeam, capture)
  if (capture) then
    return true
  end
  if (not enabled) then
    return true
  end

  local ud = UnitDefs[unitDefID]
  if (not ud) then
    return true  -- something is borked
  end

  -- compute the share delay
  local cost = ud.metalCost + (ud.energyCost / 60)
  local costDelay = math.floor(cost * costScale)
  local shareDelay = minDelay + costDelay

  local team = teams[oldTeam]
  if ((team == nil) and (shareDelay <= 0)) then
    return true  -- share the unit immediately
  end

  InsertShare(unitID, oldTeam, newTeam, shareDelay)

  return false
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function gadget:GameFrame(frameNum)
  for _,team in pairs(teams) do
    while (team and (#team > 0)) do
      local front = team[1]
      if (front.frame > frameNum) then
        break  -- front is not yet ready to be shared
      end

      local curTeam = Spring.GetUnitTeam(front.unitID)
      if (curTeam and (curTeam == front.oldTeam)) then
        -- FIXME: see if newTeam is alive
        local tmp = AllowUnitTransfer
        AllowUnitTransfer = function() return true end
        Spring.TransferUnit(front.unitID, front.newTeam)
        AllowUnitTransfer = tmp
      end

      RemoveShare(front.unitID)
    end
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function gadget:AllowCommand(unitID, unitDefID, unitTeam,
                             cmdID, cmdParams, cmdOptions)
  if (cmdID == CMD_CANCEL_SHARE) then
    RemoveShare(unitID)
    return false
  end
  return true
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function gadget:UnitDestroyed(unitID, unitDefID, unitTeam)
  RemoveShare(unitID)
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function gadget:UnitTaken(unitID)
  RemoveShare(unitID)
end


--------------------------------------------------------------------------------
--  SYNCED
--------------------------------------------------------------------------------
else
--------------------------------------------------------------------------------
--  UNSYNCED
--------------------------------------------------------------------------------

function gadget:Initialize()
end


function gadget:Shutdown()
end


local GetGameFrame		   = Spring.GetGameFrame
local GetUnitPosition    = Spring.GetUnitPosition
local GetUnitAllyTeam    = Spring.GetUnitAllyTeam
local GetLocalAllyTeamID = Spring.GetLocalAllyTeamID
local AddWorldIcon       = Spring.AddWorldIcon
local AddWorldText       = Spring.AddWorldText

function gadget:DrawWorld()
  local frames = SYNCED.shareFrames
  if ((frames == nil) or (snext(frames) == nil)) then
    return
  end
  local nowFrame = GetGameFrame()
  local myAllyTeam = GetLocalAllyTeamID()
  for unitID, frame in spairs(frames) do
    if (GetUnitAllyTeam(unitID) == myAllyTeam) then
      local x, y, z = GetUnitPosition(unitID)
      if (x) then
        local str = string.format('%.1f', (frame - nowFrame) / Game.gameSpeed)
        AddWorldIcon(x, y, z, CMD.STOP)
        AddWorldText(str, x, y, z)
      end
    end
  end
end


--------------------------------------------------------------------------------
--  UNSYNCED
--------------------------------------------------------------------------------
end
--------------------------------------------------------------------------------
--  COMMON
--------------------------------------------------------------------------------
