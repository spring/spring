--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    tweakmode.lua
--  brief:   provides default tweak mode call-ins for widgets
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

--
--  This is a work in progress...
--

include("keysym.h.lua")


local xmin = 0
local ymin = 0
local xmax = 400
local ymax = 400

local xscale = 1
local yscale = 1
local xyscale = 1

local xminOld = 0
local yminOld = 0
local xmaxOld = 0
local ymaxOld = 0
local xscaleOld = 1
local yscaleOld = 1
local xyscaleOld = 1

local xminsize = 10
local yminsize = 10

local mouseScale = 1

local UpdateHook = nil

local xside = 0
local yside = 0


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function InstallTweakMode(updateHook)
  UpdateHook = updateHook
end


local function StartTweak()
  xminOld    = xmin
  yminOld    = ymin
  xmaxOld    = xmax
  ymaxOld    = ymax
  xscaleOld  = xscale
  yscaleOld  = yscale
  xyscaleOld = xyscale
end


local function RevertTweak()
  xmin    = xminOld
  xmax    = xmaxOld
  ymin    = yminOld
  ymax    = ymaxOld
  xscale  = xscaleOld
  yscale  = yscaleOld
  xyscale = xyscaleOld
end


local function SendUpdate()
  
  if true then return end -- ???
  
  if (UpdateHook == nil) then
    print('ERROR')
    error('UpdateHook == nil, '..widget.whInfo.basename)
  end
  UpdateHook(xmin, ymin, xmax, ymax, xscale, yscale, xyscale)
end


local function IsAbove(x, y)
  return ((x >= xmin) and (x <= xmax) and (y >= ymin) and (y <= ymax))
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:TweakMousePress(x, y, button)
  if (not IsAbove(x, y)) then
    return false
  end
  local alt,ctrl,meta,shift = Spring.GetModKeyState()
  if (ctrl) then
    if (button == 1) then
      widgetHandler:RaiseWidget()
    elseif (button == 3) then
      widgetHandler:LowerWidget()
    end
  end
  StartTweak()
  if (button == 3) then
    local xsize = (xmax - xmin)
    local ysize = (ymax - ymin)
    local xmid  = (xmax + xmin) * 0.5
    local ymid  = (ymax + ymin) * 0.5
        if (x < (xmid - (0.15 * xsize))) then
      xside = -1
    elseif (x > (xmid + (0.15 * xsize))) then
      xside = 1
    else
      xside = 0
    end
        if (y < (ymid - (0.15 * ysize))) then
      yside = -1
    elseif (y > (ymid + (0.15 * ysize))) then
      yside = 1
    else
      yside = 0
    end
  end
  return true
end


function widget:TweakMouseMove(x, y, dx, dy, button)
--  print('TWEAK (TweakMouseMove) '..x..' '..y..' '..dx..' '..dy..' '..button)
  if (not widgetHandler:IsMouseOwner()) then
    return false
  end
  
  local vsx,vsy = widgetHandler:GetViewSizes()
  if (button == 1) then
    if ((xmin + dx) < 0) then dx = - xmin end
    if ((ymin + dy) < 0) then dy = - ymin end
    if ((xmax + dx) > vsx) then dx = vsx - xmax end
    if ((ymax + dy) > vsy) then dy = vsy - ymax end
    xmin = xmin + dx
    xmax = xmax + dx
    ymin = ymin + dy
    ymax = ymax + dy
    SendUpdate()
  elseif (button == 3) then
    if (xside == -1) then
      xmin = xmin + dx
      if (xmin < 0) then xmin = 0 end
      if ((xmax - xmin) < xminsize) then xmin = xmax - xminsize end
    elseif (xside == 1) then
      xmax = xmax + dx
      if (xmax > vsx) then xmax = vsx end
      if ((xmax - xmin) < xminsize) then xmax = xmin + xminsize end
    end

    if (yside == -1) then
      ymin = ymin + dy
      if (ymin < 0) then ymin = 0 end
      if ((ymax - ymin) < yminsize) then ymin = ymax - yminsize end
    elseif (yside == 1) then
      ymax = ymax + dy
      if (ymax > vsy) then ymax = vsy end
      if ((ymax - ymin) < yminsize) then ymax = ymin + yminsize end
    end
    SendUpdate()
  end
  return true
end


function widget:TweakMouseRelease(x, y, button)
  print('TWEAK (TweakMouseRelease) '..x..' '..y..' '..button)
  return -1
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:TweakIsAbove(x, y)
  return IsAbove(x, y)
end


function widget:TweakGetTooltip(x, y)
  return 'LMB = move\n'..
         'RMB = size\n'..
         'LMB+CTRL = raise\n'..
         'RMB+CTRL = lower'
end


--------------------------------------------------------------------------------

function widget:TweakKeyPress(key, mods, isRepeat)
  print('TWEAK (TweakKeyPress) '..key)
  return false
end


function widget:TweakKeyRelease(key, mods)
  print('TWEAK (TweakKeyRelease) '..key)
  if (key == KEYSYMS.ESCAPE) then
    RevertTweak()
    SendUpdate()
    widgetHandler:DisownMouse()
    return true
  end
  return false
end


--------------------------------------------------------------------------------

function widget:TweakDrawScreen()
  local x,y = Spring.GetMouseState()
  if (not widgetHandler:IsMouseOwner() and not IsAbove(x, y)) then
    return
  end
  
  -- ??? add an indicator for xside/yside
  
  gl.Blending(GL.SRC_ALPHA, GL.ONE)
  gl.Color(0.8, 0.8, 1.0, 0.25)
  gl.Shape(GL.QUADS, {
    { v = { xmin, ymin } }, { v = { xmax, ymin } }, 
    { v = { xmax, ymax } }, { v = { xmin, ymax } }
  })
  gl.Color(0.0, 0.0, 1.0, 0.5)
  gl.Shape(GL.QUADS, {
    { v = { xmin + 3, ymin + 3} }, { v = { xmax - 3, ymin + 3 } }, 
    { v = { xmax - 3, ymax - 3} }, { v = { xmin + 3, ymax - 3 } }
  })
  gl.Color(1.0, 1.0, 1.0)
  gl.Blending(GL.SRC_ALPHA, GL.ONE_MINUS_SRC_ALPHA)
end


--------------------------------------------------------------------------------
