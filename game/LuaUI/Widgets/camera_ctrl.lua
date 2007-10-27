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
    name      = "CameraControl",
    desc      = "Low level access to camera parameters",
    author    = "trepan",
    date      = "Mar 21, 2007",
    license   = "GNU GPL, v2 or later",
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

local function GetCamereTable()
  local cs = Spring.GetCameraState()
  if (cs == nil) then
    return nil
  end

  local ct = {
    px = 2, py = 3, pz = 4  --  available to all modes, and 1 is 'num'
  }
  
  if (cs.name == 'ov') then    -- Overview
    ct.fullName = "Overview"
  elseif ((cs.name == 'fps') or (cs.name == 'rot')) then   -- FPS and Rotating
    if (cs.name == 'fps') then
      ct.fullName = "First Person Shooter (FPS)"
    else
      ct.fullName = "Rotatable Camera"
    end
    ct.dx        = 5
    ct.dy        = 6
    ct.dz        = 7
    ct.rx        = 8
    ct.ry        = 9
    ct.rz        = 10
    ct.oldHeight = 11
  elseif (cs.name == 'ta') then    -- Total Annihilation
    ct.fullName = "Total Annihilation Overhead Camera"
    ct.dx      = 5
    ct.dy      = 6
    ct.dz      = 7
    ct.height  = 8
    ct.zscale  = 9
    ct.flipped = 10
  elseif (cs.name == 'tw') then    -- Total War 
    ct.fullName = "Total War Camera"
    ct.rx = 5
    ct.ry = 6
    ct.rz = 7
  elseif (cs.name == 'free') then  -- Free Camera
    ct.fullName = "Free Camera"
    ct.dx          = 5
    ct.dy          = 6
    ct.dz          = 7
    ct.rx          = 8
    ct.ry          = 9
    ct.rz          = 10
    ct.fov         = 11
    ct.gndOffset   = 12
    ct.gravity     = 13
    ct.slide       = 14
    ct.scrollSpeed = 15
    ct.tiltSpeed   = 16
    ct.velTime     = 17
    ct.avelTime    = 18
    ct.autoTilt    = 19
    ct.goForward   = 20
    ct.invertAlt   = 21
    ct.gndLock     = 22
    ct.vx          = 23
    ct.vy          = 24
    ct.vz          = 25
    ct.avx         = 26
    ct.avy         = 27
    ct.avz         = 28
  end
  return ct, cs
end


--------------------------------------------------------------------------------

local function CamCtrl(cmd, line, words)
  local wc = #words
  local ct, cs = GetCamereTable()
  if (ct == nil) then
    Spring.Echo("Couldn't get the camera state")
    return true
  end

  if (wc == 0) then  --  print the parameters names
    Spring.Echo(ct.fullName .. " Parameters:")
    local array = {}
    for k,v in pairs(ct) do
      if (type(v) == "number") then
        table.insert(array, k)
      end
    end
    table.sort(array)
    Spring.Echo(table.concat(array, " "))

  elseif (wc == 1) then  --  print a parameters value
    Spring.Echo(ct.fullName)
    local param = ct[words[1]]
    if ((param == nil) or (type(param) ~= 'number')) then
      Spring.Echo("  bad parameter name: " .. words[1])
      return true
    end
    Spring.Echo("  " .. words[1] .. " = " .. tostring(cs[param]))

  elseif (wc >= 2) then  --  set a parameters value
    Spring.Echo(ct.fullName)
    local param = ct[words[1]]
    if ((param == nil) or (type(param) ~= 'number')) then
      Spring.Echo("  bad parameter name: " .. words[1])
      return true
    end
    local value = tonumber(words[2])
    if (value == nil) then
      Spring.Echo("  bad value: " .. words[2])
      return true
    end
    cs[param] = tonumber(value)
    local camTime = 0
    if (wc >= 3) then  --  specified transition time?
      camTime = tonumber(words[3])
      if (camTime == nil) then
        Spring.Echo("  bad camTime: " .. words[3])
        return true
      end
    end
    Spring.SetCameraState(cs, camTime)
    Spring.Echo("  set " .. words[1] .. " to " .. value)
  end

  return true
end


function widget:Initialize()
  widgetHandler:AddAction("cam", CamCtrl)
end


--------------------------------------------------------------------------------

