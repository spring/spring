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

-- Automatically generated local definitions

local glColor                 = gl.Color
local glText                  = gl.Text
local spGetCurrentTooltip     = Spring.GetCurrentTooltip
local spGetSelectedUnitsCount = Spring.GetSelectedUnitsCount
local spSendCommands          = Spring.SendCommands


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

local currentTooltip = ''

--------------------------------------------------------------------------------

local vsx, vsy = widgetHandler:GetViewSizes()

function widget:ViewResize(viewSizeX, viewSizeY)
  vsx = viewSizeX
  vsy = viewSizeY
end


--------------------------------------------------------------------------------

function widget:Initialize()
  spSendCommands({"tooltip 0"})
end


function widget:Shutdown()
  spSendCommands({"tooltip 1"})
end


--------------------------------------------------------------------------------

local magic = '\001'

function widget:WorldTooltip(ttType, data1, data2, data3)
--  do return end
  if (ttType == 'unit') then
    return magic .. 'unit #' .. data1
  elseif (ttType == 'feature') then
    return magic .. 'feature #' .. data1
  elseif (ttType == 'ground') then
    return magic .. string.format('ground @ %.1f %.1f %.1f',
                                  data1, data2, data3)
  elseif (ttType == 'selection') then
    return magic .. 'selected ' .. spGetSelectedUnitsCount()
  else
    return 'WTF? ' .. '\'' .. tostring(ttType) .. '\''
  end
end


if (true) then
  widget.WorldTooltip = nil
end


--------------------------------------------------------------------------------

function widget:DrawScreen()
  if (fh) then
    fh = fontHandler.UseFont(fontName)
    glColor(1, 1, 1)
  end
  local white = "\255\255\255\255"
  local bland = "\255\211\219\255"
  local mSub, eSub
  local tooltip = spGetCurrentTooltip()

  if (string.sub(tooltip, 1, #magic) == magic) then
    tooltip = 'WORLD TOOLTIP:  ' .. tooltip
  end

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

  for line in string.gmatch(tooltip, "([^\n]*)\n?") do
    if (unitTip and (i == 0)) then
      line = "\255\255\128\255" .. line
    else
      line = "\255\255\255\255" .. line
    end
    
    if (fh) then
      fontHandler.Draw(line, gap, gap + (4 - i) * yStep)
    else
      glText(line, gap, gap + (4 - i) * yStep, fontSize, "o")
    end

    i = i + 1
  end

  if (disableCache) then
    fontHandler.EnableCache()
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
