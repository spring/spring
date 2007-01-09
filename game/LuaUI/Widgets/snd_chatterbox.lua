--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    snd_chatterbox.lua
--  brief:   annoys sounds
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "ChatterBox",
    desc      = "annoying sounds",
    author    = "trepan",
    date      = "Jan 8, 2007",
    license   = "GNU GPL, v2 or later",
    layer     = -10,
    enabled   = false  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local SOUND_DIRNAME = LUAUI_DIRNAME .. 'Sounds/'


local function playSound(filename, ...)
  Spring.PlaySoundFile(SOUND_DIRNAME .. filename, unpack(arg))
end


--------------------------------------------------------------------------------

function widget:KeyPress(key, mods, isRepeat)
  if (not isRepeat) then
    playSound('flag_grab.wav')
  else
    playSound('land.wav', 0.4)
  end
  return false
end

--[[
function widget:KeyRelease(key, mods)
  playSound('flag_grab.wav')
  return false
end
--]]


function widget:MousePress(x, y, button)
  if     (button == 1) then playSound('pop.wav')
  elseif (button == 2) then playSound('message_admin.wav')
  elseif (button == 3) then playSound('bounce.wav')
  end
  return false
end


function widget:MouseRelease(x, y, button)
  playSound('teamgrab.wav')
  return -1
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  UnitReady() events can come from multiple builders, and the arrival of
--  the events from some of the builders can be delayed. We avoid playing
--  multiple sounds for the same new unit by storing a timecheck to keep
--  track of the last time this unit id generated a sound.
--

local unitReady = false
local unitTable = {}


function widget:UnitReady(unitID, unitDefID, builderID, builderUnitDefID)
  if (Spring.GetUnitTeam(unitID) == Spring.GetMyTeamID()) then
    local nowTime = Spring.GetGameSeconds()
    local nextTime = unitTable[unitID]
    if (not nextTime or (nowTime > nextTime)) then
      unitReady = true
      unitTable[unitID] = nowTime + 120  --  nextTime
    end
  end
end


function widget:Update(deltaTime)
  if (unitReady) then
    unitReady = false
    playSound('teamgrab.wav')
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
