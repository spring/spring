--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    camera_smooth_scroll.lua
--  brief:   Alternate camera scrolling for the middle mouse button
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "SmoothScroll",
    desc      = "Alternate view movement for the middle mouse button",
    author    = "trepan",
    date      = "Feb 27, 2007",
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
--  1. The GetCameraState() table is supposed to be opaque.
--

local gl = gl


local vsx, vsy = widgetHandler:GetViewSizes()
function widget:ViewResize(viewSizeX, viewSizeY)
  vsx = viewSizeX
  vsy = viewSizeY
end


local decayFactor = 1
local speedFactor = 25

local mx, my
local active = false


--------------------------------------------------------------------------------

function widget:Update(dt)
  if (active) then
    local x, y, lmb, mmb, rmb = Spring.GetMouseState()
    local cs = Spring.GetCameraState()
    local speed = dt * speedFactor

    if (cs.name == 'free') then
      local a,c,m,s = Spring.GetModKeyState()
      if (c) then
        return
      end
    end
    if (cs.name == "ta") then
      local flip = -cs[10]
      -- simple, forward and right are locked
      cs[2] = cs[2] + (speed * flip * (x - mx))
      if (false) then
        cs[3] = cs[3] + (speed * flip * (my - y))
      else
        cs[4] = cs[4] + (speed * flip * (my - y))
      end
    else
      -- forward, up, right, top, bottom, left, right
      local camVecs = Spring.GetCameraVectors()
      local cf = camVecs.forward
      local len = math.sqrt((cf[1] * cf[1]) + (cf[3] * cf[3]))
      local dfx = cf[1] / len
      local dfz = cf[3] / len
      local cr = camVecs.right
      local len = math.sqrt((cr[1] * cr[1]) + (cr[3] * cr[3]))
      local drx = cr[1] / len
      local drz = cr[3] / len
      local mxm = (speed * (x - mx))
      local mym = (speed * (y - my))
      cs[2] = cs[2] + (mxm * drx) + (mym * dfx)
      cs[4] = cs[4] + (mxm * drz) + (mym * dfz)
    end
    Spring.SetCameraState(cs, 0)
    if (mmb) then
      Spring.SetMouseCursor("none")
    end
  end
end


function widget:MousePress(x, y, button)
  if (button ~= 2) then
    return false
  end
  local cs = Spring.GetCameraState()
  if ((cs.name ~= "ta") and (cs.name ~= "free")) then
--  if ((cs.name ~= "ta") and (cs.name ~= "rot") and (cs.name ~= "free")) then
    local a,c,m,s = Spring.GetModKeyState()
    return (c or s)  --  block the mode toggling
  end
  if (cs.name == 'free') then
    local a,c,m,s = Spring.GetModKeyState()
    if (m) then
      return
    end
  end
  active = not active
  if (active) then
    mx = vsx * 0.5
    my = vsy * 0.5
    Spring.WarpMouse(mx, my)
    Spring.SendCommands({"trackoff"})
  end
  return true
end


function widget:MouseRelease(x, y, button)
  active = false
  return -1
end


function widget:DrawScreen()
  if (active) then
    local x, y = Spring.GetMouseState()

    gl.Color(0, 0, 0)
    gl.PointSize(14)
    gl.Shape(GL.POINTS, { { v = { mx, my },  c = {0, 0, 0} } })
    gl.PointSize(11)
    gl.Shape(GL.POINTS, { { v = { mx, my },  c = {1, 1, 1} } })
    gl.PointSize(8)
    gl.Shape(GL.POINTS, { { v = { mx, my },  c = {0, 0, 0} } })
    gl.PointSize(5)
    gl.Shape(GL.POINTS, { { v = { mx, my },  c = {1, 0, 0} } })

    gl.LineWidth(2)
    gl.Shape(GL.LINES, {
      { v = {  x,  y }, c = {0, 1, 0} },
      { v = { mx, my }, c = {1, 0, 0} }
    })
    gl.LineWidth(1)

    gl.PointSize(5)
    gl.Shape(GL.POINTS, { { v = {  x,  y},  c = {0, 1, 0} } })
    gl.PointSize(1)
  end
end


--------------------------------------------------------------------------------

