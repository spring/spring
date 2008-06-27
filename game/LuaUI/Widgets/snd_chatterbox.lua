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
    desc      = "Annoying sounds",
    author    = "trepan",
    date      = "Jan 9, 2007",
    license   = "GNU GPL, v2 or later",
    layer     = -10,
    enabled   = false  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local SOUND_DIRNAME = LUAUI_DIRNAME .. 'Sounds/'


local function playSound(filename, ...)
  Spring.PlaySoundFile(SOUND_DIRNAME .. filename, ...)
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


function widget:UnitFinished(unitID, unitDefID, unitTeam)
  if (unitTeam == Spring.GetMyTeamID()) then
    playSound('teamgrab.wav')
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
