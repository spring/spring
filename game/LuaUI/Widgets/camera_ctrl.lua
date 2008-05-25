--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    camera_ctrl.lua
--  brief:   
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = 'CameraControl',
    desc      = 'Low level access to camera parameters',
    author    = 'trepan',
    date      = 'Mar 21, 2007',
    license   = 'GNU GPL, v2 or later',
    layer     = 1,     --  after the normal widgets
    enabled   = false  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  NOTES:
--
--    The GetCameraState() table is supposed to be opaque.   :-)
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local fullNames = {
  fps  = 'First Person Shooter (FPS)',
  free = 'Free Camera',
  ov   = 'Overview',
  rot  = 'Rotatable Camera',
  ta   = 'Total Annihilation Overhead Camera',
  tw   = 'Total War Camera',
}


local function GetFullName(cs)
  local fullName = fullNames[cs.name]
  if (fullName) then
    return fullName
  end
  return cs.name
end


--------------------------------------------------------------------------------

local function CamCtrl(cmd, line, words)
  local wc = #words
  local cs = Spring.GetCameraState()

  if (cs == nil) then
    Spring.Echo('Could not get the camera state')
    return true
  end

  if (wc == 0) then  --  print the parameters names
    Spring.Echo(GetFullName(cs) .. ' Parameters:')
    local array = {}
    for k,v in pairs(cs) do
      if ((type(k) == 'string') and (k ~= 'name')) then
        table.insert(array, k)
      end
    end
    table.sort(array)
    Spring.Echo(table.concat(array, ' '))

  elseif (wc == 1) then  --  print a parameters value
    Spring.Echo(GetFullName(cs))
    local value = cs[words[1]]
    if (type(value) ~= 'number') then
      Spring.Echo('  bad parameter name: ' .. words[1])
      return true
    end
    Spring.Echo('  ' .. words[1] .. ' = ' .. value)

  elseif (wc >= 2) then  --  set a parameters value
    Spring.Echo(GetFullName(cs))
    local value = cs[words[1]]
    if (type(value) ~= 'number') then
      Spring.Echo('  bad parameter name: ' .. words[1])
      return true
    end
    cs[words[1]] = tonumber(words[2])
    local camTime = 0
    if (wc >= 3) then  --  specified transition time?
      camTime = tonumber(words[3])
      if (camTime == nil) then
        Spring.Echo('  bad camTime: ' .. words[3])
        return true
      end
    end
    Spring.SetCameraState(cs, camTime)
    Spring.Echo('  set ' .. words[1] .. ' to ' .. value)
  end

  return true
end


function widget:Initialize()
  widgetHandler:AddAction('cam', CamCtrl)
end


--------------------------------------------------------------------------------

