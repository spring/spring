--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    gui_tooltip.lua
--  brief:   recolors some of the tooltip info
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "Tooltip",
    desc      = "A colorful tooltip modification",
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


local fontSize = 12
local ySpace   = 4
local yStep    = fontSize + ySpace
local gap      = 4

local fh = (1 > 0)
local fontName  = "LuaUI/Fonts/FreeSansBold_14"
if (fh) then
  fh = fontHandler.UseFont(fontName)
end
if (fh) then
  fontSize  = fontHandler.GetFontSize()
  yStep     = fontHandler.GetFontYStep() + 2
end


--------------------------------------------------------------------------------

local vsx, vsy = widgetHandler:GetViewSizes()

function widget:ViewResize(viewSizeX, viewSizeY)
  vsx = viewSizeX
  vsy = viewSizeY
end


--------------------------------------------------------------------------------

function widget:Initialize()
  Spring.SendCommands({"tooltip 0"})
end


function widget:Shutdown()
  Spring.SendCommands({"tooltip 1"})
end


--------------------------------------------------------------------------------

function widget:DrawScreen()
  if (fh) then
    fh = fontHandler.UseFont(fontName)
    gl.Color(1, 1, 1)
  end
  local white = "\255\255\255\255"
  local bland = "\255\211\219\255"
  local mSub, eSub
  local tooltip = Spring.GetCurrentTooltip()
  tooltip, mSub = string.gsub(tooltip, bland.."Me",   "\255\1\255\255Me")
  tooltip, eSub = string.gsub(tooltip, bland.."En", "  \255\255\255\1En")
  tooltip = string.gsub(tooltip,
                        "Hotkeys:", "\255\255\128\128Hotkeys:\255\128\192\255")
  tooptip =       string.gsub(tooltip, "a", "b")
  local unitTip = ((mSub + eSub) == 2)
  local i = 0

  local disableCache = (fh and string.find(tooltip, "^Pos"))
  if (disableCache) then
    fontHandler.DisableCache()  -- ground tooltips change too much for caching
  end

  for line in string.gfind(tooltip, "([^\n]*)\n?") do
    if (unitTip and (i == 0)) then
      line = "\255\255\128\255" .. line
    else
      line = "\255\255\255\255" .. line
    end
    
    if (fh) then
      fontHandler.Draw(line, gap, gap + (4 - i) * yStep)
    else
      gl.Text(line, gap, gap + (4 - i) * yStep, fontSize, "o")
    end

    i = i + 1
  end

  if (disableCache) then
    fontHandler.EnableCache()
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
