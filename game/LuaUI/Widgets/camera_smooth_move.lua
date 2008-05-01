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

-- Automatically generated local definitions

local GL_LINES           = GL.LINES
local GL_POINTS          = GL.POINTS
local glBeginEnd         = gl.BeginEnd
local glColor            = gl.Color
local glLineWidth        = gl.LineWidth
local glPointSize        = gl.PointSize
local glVertex           = gl.Vertex
local spGetCameraState   = Spring.GetCameraState
local spGetCameraVectors = Spring.GetCameraVectors
local spGetModKeyState   = Spring.GetModKeyState
local spGetMouseState    = Spring.GetMouseState
local spSendCommands     = Spring.SendCommands
local spSetCameraState   = Spring.SetCameraState
local spSetMouseCursor   = Spring.SetMouseCursor
local spWarpMouse        = Spring.WarpMouse


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  NOTES:
--
--  1. The GetCameraState() table is supposed to be opaque.
--

local blockModeSwitching = true


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
    local x, y, lmb, mmb, rmb = spGetMouseState()
    local cs = spGetCameraState()
    local speed = dt * speedFactor

    if (cs.name == 'free') then
      local a,c,m,s = spGetModKeyState()
      if (c) then
        return
      end
      -- clear the velocities
      for p = 23,28 do
        cs[p] = 0
      end
    end
    if (cs.name == "ta") then
      local flip = -cs[9]
      -- simple, forward and right are locked
      cs[1] = cs[1] + (speed * flip * (x - mx))
      if (false) then
        cs[2] = cs[2] + (speed * flip * (my - y))
      else
        cs[3] = cs[3] + (speed * flip * (my - y))
      end
    else
      -- forward, up, right, top, bottom, left, right
      local camVecs = spGetCameraVectors()
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
    spSetCameraState(cs, 0)
    if (mmb) then
      spSetMouseCursor("none")
    end
  end
end


function widget:MousePress(x, y, button)
  if (button ~= 2) then
    return false
  end
  local cs = spGetCameraState()
  if (blockModeSwitching and (cs.name ~= "ta") and (cs.name ~= "free")) then
    local a,c,m,s = spGetModKeyState()
    return (c or s)  --  block the mode toggling
  end
  if (cs.name == 'free') then
    local a,c,m,s = spGetModKeyState()
    if (m and (not (c or s))) then
      return false
    end
  end
  active = not active
  if (active) then
    mx = vsx * 0.5
    my = vsy * 0.5
    spWarpMouse(mx, my)
    spSendCommands({"trackoff"})
  end
  return true
end


function widget:MouseRelease(x, y, button)
  active = false
  return -1
end


local function DrawPoint(x, y, c, s)
  glPointSize(s)
  glColor(c)
  glBeginEnd(GL_POINTS, function(x, y)
    glVertex(x, y)
  end, x, y)
end


function widget:DrawScreen()
  if (active) then
    local x, y = spGetMouseState()

    DrawPoint(mx, my, { 0, 0, 0 }, 14)
    DrawPoint(mx, my, { 1, 1, 1 }, 11)
    DrawPoint(mx, my, { 0, 0, 0 },  8)
    DrawPoint(mx, my, { 1, 0, 0 },  5)

    glLineWidth(2)
    glBeginEnd(GL_LINES, function()
      glColor(0, 1, 0); glVertex(x,  y)
      glColor(1, 0, 0); glVertex(mx, my)
    end)
    glLineWidth(1)

    DrawPoint(x, y, { 0, 1, 0 },  5)

    glPointSize(1)
  end
end


--------------------------------------------------------------------------------

