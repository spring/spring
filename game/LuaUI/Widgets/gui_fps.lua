--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    gui_fps.lua
--  brief:   displays the current frames-per-seconds
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "FPS",
    desc      = "Displays the current frames-per-second",
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
include("tweakmode.lua")


local floor = math.floor


local vsx, vsy = widgetHandler:GetViewSizes()

-- the 'f' suffixes are fractions  (and can be nil)
local color  = { 1.0, 1.0, 0.25 }
local xposf  = 0.99
local xpos   = xposf * vsx
local yposf  = 0.032
local ypos   = yposf * vsy
local sizef  = 0.015
local size   = sizef * vsy
local font   = "LuaUI/Fonts/FreeSansBold_14"
local format = "orn"

local fh = (font ~= nil)


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Rendering
--

function widget:DrawScreen()
  gl.Color(color)
  if (fh) then
    fh = fontHandler.UseFont(font)
    fontHandler.DisableCache()
    fontHandler.DrawRight(Spring.GetFPS(), floor(xpos), floor(ypos))
    fontHandler.EnableCache()
  else
    gl.Text(Spring.GetFPS(), xpos, ypos, size, format)
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
