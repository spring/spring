--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    gui_clock.lua
--  brief:   displays the current game time
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "Clock",
    desc      = "Shows the current game time",
    author    = "trepan",
    date      = "Jan 8, 2007",
    license   = "GNU GPL, v2 or later",
    layer     = 0,
    enabled   = false  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

include("colors.h.lua")


local floor = math.floor


local vsx, vsy = widgetHandler:GetViewSizes()

-- the 'f' suffixes are fractions  (and can be nil)
local color  = { 1.0, 1.0, 1.0 }
local xposf  = 0.99
local xpos   = xposf * vsx
local yposf  = 0.010
local ypos   = yposf * vsy
local sizef  = 0.015
local size   = sizef * vsy
local font   = "LuaUI/Fonts/FreeSansBold_14"
local format = "orn"

local fh = (font ~= nil)

local timeSecs = 0
local timeString = "00:00"


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Default GUI override
--

local defaultClockUsed = 0


function widget:Initialize()
  defaultClockUsed = Spring.GetConfigInt("ShowClock", 1)
  Spring.SendCommands({"clock 0"})
end


function widget:Shutdown()
  Spring.SendCommands({"clock " .. defaultClockUsed})
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Rendering
--

local function GetTimeString()
  local secs = math.floor(Spring.GetGameSeconds())
  if (timeSecs ~= secs) then
    timeSecs = secs
    local h = math.floor(secs / 3600)
    local m = math.floor((secs % 3600) / 60)
    local s = math.floor(secs % 60)
    if (h > 0) then
      timeString = string.format('%02i:%02i:%02i', h, m, s)
    else
      timeString = string.format('%02i:%02i', m, s)
    end
  end
  return timeString
end


function widget:DrawScreen()
  gl.Color(color)
  if (fh) then
    fh = fontHandler.UseFont(font)
    fontHandler.DisableCache()
    fontHandler.DrawRight(GetTimeString(), floor(xpos), floor(ypos))
    fontHandler.EnableCache()
  else
    gl.Text(GetTimeString(), xpos, ypos, size, format)
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Geometry Management
--

local function UpdateGeometry()
  -- use the fractions if available
  xpos = (xposf and (xposf * vsx)) or xpos
  ypos = (yposf and (yposf * vsy)) or ypos
  size = (sizef and (sizef * vsy)) or size
  -- negative values reference the right/top edges
  xpos = (xpos < 0) and (vsx + xpos) or xpos
  ypos = (ypos < 0) and (vsy + ypos) or ypos
end
UpdateGeometry()


function widget:ViewResize(viewSizeX, viewSizeY)
  vsx = viewSizeX
  vsy = viewSizeY
  UpdateGeometry()
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Configuration routines
--

local function StoreGeoPair(tbl, fName, fValue, pName, pValue)
  if (fValue) then
    tbl[pName] = nil
    tbl[fName] = fValue
  else
    tbl[pName] = pValue
    tbl[fName] = nil
  end
  return
end


function widget:GetConfigData()
  local tbl = {
    color  = color,
    format = format,
    font   = font
  }
  StoreGeoPair(tbl, 'xposf', xposf, 'xpos', xpos)
  StoreGeoPair(tbl, 'yposf', yposf, 'ypos', ypos)
  StoreGeoPair(tbl, 'sizef', sizef, 'size', size)
  return tbl
end


--------------------------------------------------------------------------------

-- returns a fraction,pixel pair
local function LoadGeoPair(tbl, fName, pName, oldPixelValue)
  if     (tbl[fName]) then return tbl[fName],      1
  elseif (tbl[pName]) then return nil,    tbl[pName]
  else                     return nil, oldPixelValue
  end
end


function widget:SetConfigData(data)
  color  = data.color  or color
  format = data.format or format
  font   = data.font   or font
  if (font) then
    fh = fontHandler.UseFont(font)
  end
  xposf, xpos = LoadGeoPair(data, 'xposf', 'xpos', xpos)
  yposf, ypos = LoadGeoPair(data, 'yposf', 'ypos', ypos)
  sizef, size = LoadGeoPair(data, 'sizef', 'size', size)
  UpdateGeometry()
  return
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
