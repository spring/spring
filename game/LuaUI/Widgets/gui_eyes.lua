--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    gui_eyes.lua
--  brief:   XEyes clone
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "Eyes",
    desc      = "XEyes clone",
    author    = "trepan",
    date      = "Jan 8, 2007",
    license   = "GNU GPL, v2 or later",
    layer     = -5,
    enabled   = true  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
-- Useless xeyes clone
--

include("colors.h.lua")


local gl = gl  --  use a local copy for faster access

local divs = 64
local eyeGap = 8
local lineWidth = 4

local vsx, vsy = widgetHandler:GetViewSizes()

local lEye = {
  x = 0, y = 0, sx = 60, sy = 90, ps = 0.25, px = 0, py = 0, td = -1
}
local rEye = {
  x = 0, y = 0, sx = 60, sy = 90, ps = 0.25, px = 0, py = 0, td =  1
}

local alpha = 0.8
local white  = { 0.8, 0.8, 1.0, alpha }
local black  = { 0.0, 0.0, 0.0, alpha }


local function UpdateGeometry(viewSizeX, viewSizeY)
  local gap = 8
  vsx, vsy = viewSizeX, viewSizeY
  lEye.x = (vsx * 0.25) - (0.5 * (lEye.sx + eyeGap + lineWidth))
  rEye.x = (vsx * 0.25) + (0.5 * (rEye.sx + eyeGap + lineWidth))
  lEye.y = (vsy * 0.20)
  rEye.y = (vsy * 0.20)
  -- center the pupils
  lEye.px, lEye.py = lEye.x, lEye.y
  rEye.px, rEye.py = rEye.x, rEye.y
end
UpdateGeometry(vsx, vsy)


function widget:ViewResize(viewSizeX, viewSizeY)
  UpdateGeometry(viewSizeX, viewSizeY)
end


local function MakeCircleList(type)
  local elements = {}
  for i = 0,divs-1 do
    local r = 2.0 * math.pi * (i / divs)
    table.insert(elements, {
      v = { math.cos(r), math.sin(r) },
      t = { math.cos(r), math.sin(r) }
    })
  end
  gl.Shape(type, elements)
end
local circleLineList = gl.CreateList(MakeCircleList, GL.LINE_LOOP)
local circlePolyList = gl.CreateList(MakeCircleList, GL.TRIANGLE_FAN)


function widget:GetConfigData(data)
  return {
    divs      = divs,
    eyeGap    = eyeGap,
    lineWidth = lineWidth,
    leftEye   = lEye,
    rightEye  = rEye,
    alpha     = alpha,
    white     = { white[1], white[2], white[3] },
    black     = { black[1], black[2], black[3] }
  }
end


function widget:SetConfigData(data)
  divs      = data.divs      or divs
  eyeGap    = data.eyeGap    or eyeGap
  lineWidth = data.lineWidth or lineWidth
  lEye      = data.leftEye   or lEye
  rEye      = data.rightEye  or rEye
  alpha     = data.alpha     or alpha
  white     = data.white     or white
  black     = data.black     or black
  white[4]  = alpha
  black[4]  = alpha
  UpdateGeometry(widgetHandler:GetViewSizes())

  gl.DeleteList(circleLineList)
  gl.DeleteList(circlePolyList)
  circleLineList = gl.CreateList(MakeCircleList, GL.LINE_LOOP)
  circlePolyList = gl.CreateList(MakeCircleList, GL.TRIANGLE_FAN)
end


--------------------------------------------------------------------------------

function widget:Shutdown()
  gl.DeleteList(circleLineList)
  gl.DeleteList(circlePolyList)
end


local function IsAboveEye(mx, my, e)
  local dx = (mx - e.x)
  local dy = (my - e.y)
  local dyE = dy * (e.sx / e.sy)  --  effective delta-Y
  local distsqr = (dx*dx + dyE*dyE)
  local rad = (0.5 * e.sx)
  return (distsqr <= (rad * rad))
end


function widget:IsAbove(x, y)
  return IsAboveEye(x, y, lEye) or IsAboveEye(x, y, rEye)
end


function widget:TweakIsAbove(x, y)
  return IsAboveEye(x, y, lEye) or IsAboveEye(x, y, rEye)
end


function widget:GetTooltip(x, y)
  local r = 0.333
  local toggle = ((widgetHandler:GetHourTimer() % r) < (r * 0.5))
  local cs1 = (toggle and '\255\64\255\64') or '\255\64\128\64'
  local w = WhiteStr
  local tt = w
  local tt = tt.."Use "..cs1.."F11"..w.." to enable/disable widgets\n"
  return tt
end


function widget:MousePress(x, y, button)
  -- block the locked-view mode
  return (self:IsAbove(x, y) and (button == 2))
end


function widget:TweakMousePress(x, y, button)
  if (self:IsAbove(x, y)) then
    print('widget:MouseEditPress')
    if (button == 1) then
      widgetHandler:RaiseWidget()
      return true
    elseif (button == 3) then
      widgetHandler:LowerWidget()
      return true
    end
  end
  return false
end


--------------------------------------------------------------------------------
--
--  Update 
--

local function UpdateEye(e, dt)
  local mx,my = Spring.GetMouseState()
  
  local dx = (mx - e.x)
  local dy = (my - e.y)
  local dyE = dy * (e.sx / e.sy)  --  effective delta-Y
  local dist = math.sqrt(dx*dx + dyE*dyE)
  local limit = 0.5 * (e.sx * (1.0 - e.ps) - lineWidth)

  -- udate the pupil location
  local ppx, ppy = mx, my
  if (dist > limit) then
    local scale = limit / dist
    ppx = e.x + (dx * scale)
    ppy = e.y + (dy * scale)
  end
  dt = dt * 5.0
  if (dt > 0.5) then dt = 0.5 end
  e.px = (e.px * (1.0 - dt)) + (ppx * dt)
  e.py = (e.py * (1.0 - dt)) + (ppy * dt)
end


function widget:Update(dt)
  UpdateEye(lEye, dt)
  UpdateEye(rEye, dt)
end


--------------------------------------------------------------------------------
--
--  DrawScreen
--

local function DrawLists(px, py, sx, sy, t)
  gl.PushMatrix()
  gl.Translate(px, py, 0)
  gl.Scale(sx, sy, 1)
  for k,v in pairs(t) do
    gl.Color(v[2])
    gl.CallList(v[1])
  end
  gl.PopMatrix()
end


local function DrawEye(e)
  local mx,my,lmb,mmb,rmb = Spring.GetMouseState()

  -- setup the eye color
  local color = white
  if (IsAboveEye(mx, my, e) and not widgetHandler:InTweakMode()) then
    local function B2N(v) if (v) then return 1 else return 0.2 end end
    color = { B2N(not lmb), B2N(not mmb), B2N(not rmb), alpha }
  end

  -- eye
  gl.Texture("bitmaps/graphpaper.bmp")
  local timer = widgetHandler:GetHourTimer()
  gl.MatrixMode(GL.TEXTURE)
  gl.PushMatrix()
  gl.Rotate((e.td * timer * 60) % 360, 0.0, 0.0, 1.0)
  gl.MatrixMode(GL.MODELVIEW)
  DrawLists(e.x, e.y, e.sx * 0.5, e.sy * 0.5, {
    { circlePolyList, color}, { circleLineList, black }
  })
  gl.Texture(false)
  gl.MatrixMode(GL.TEXTURE)
  gl.PopMatrix()
  gl.MatrixMode(GL.MODELVIEW)

  -- pupil
  DrawLists(e.px, e.py, 0.5 * (e.sx * e.ps), 0.5 * (e.sy * e.ps),
            {{ circlePolyList, black }})
end


function widget:DrawScreen()
  gl.LineWidth(lineWidth)
  DrawEye(lEye)
  DrawEye(rEye)
  gl.LineWidth(1.0)

  -- an unmissable message
  local htime = widgetHandler:GetHourTimer()
  local r = 0.333
  local toggle = (htime % r) < (r * 0.5)
  local cs1 = (toggle and '\255\255\255\64') or '\255\128\128\64'
  local tt = ''
  tt = tt .. WhiteStr .. 'Use'
  tt = tt .. cs1      .. ' F11'
  tt = tt .. WhiteStr .. ' to'
  tt = tt .. GreenStr .. ' enable'
  tt = tt .. GreyStr  .. '/'
  tt = tt .. RedStr   .. 'disable'
  tt = tt .. BlueStr  .. ' widgets'
  gl.PushMatrix()
  local fs = (vsy / 16)
  gl.Translate(0.5 * vsx, 0.5 * (vsy - fs), 0)
  gl.Rotate(math.cos(htime) * 60, 0, 1, 0)
  gl.Text(tt, 0, 0, fs, 'oc')
  gl.PopMatrix()
  fs = fs * 0.75
  gl.Text('\255\200\64\255'..'Brought to you by "Eyes"',
          0.5 * vsx, 0.5 * vsy - 2 * fs, fs, 'oc')
end
  

local function TweakDrawEye(e)
  gl.Blending(GL.SRC_ALPHA, GL.ONE)
  gl.Color(1.0, 1.0, 1.0, 0.125)
  gl.Shape(GL.QUADS, {
    { v = { e.x - e.sx * 0.5, e.y - e.sy * 0.5 } },
    { v = { e.x + e.sx * 0.5, e.y - e.sy * 0.5 } },
    { v = { e.x + e.sx * 0.5, e.y + e.sy * 0.5 } },
    { v = { e.x - e.sx * 0.5, e.y + e.sy * 0.5 } }
  })
  gl.Color(1, 1, 1)
  gl.Blending(GL.SRC_ALPHA, GL.ONE_MINUS_SRC_ALPHA)
end


function widget:TweakDrawScreen()
  local mx,my = Spring.GetMouseState()
  if (IsAboveEye(mx, my, lEye)) then TweakDrawEye(lEye) end
  if (IsAboveEye(mx, my, rEye)) then TweakDrawEye(rEye) end
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
