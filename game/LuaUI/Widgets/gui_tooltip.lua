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
    date      = "Jan 11, 2007",
    license   = "GNU GPL, v2 or later",
    drawLayer = 0,
    enabled   = true  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

include("colors.h.lua")
include("opengl.h.lua")


local fontSize = 12
local ySpace   = 4
local yStep    = fontSize + ySpace


function widget:Initialize()
  Spring.SendCommands({"tooltip 0"})
end


function widget:Shutdown()
  Spring.SendCommands({"tooltip 1"})
end


local vsx, vsy = widgetHandler:GetViewSizes()

function widget:ViewResize(viewSizeX, viewSizeY)
  vsx = viewSizeX
  vsy = viewSizeY
end


function widget:DrawScreen()
  local white = "\255\255\255\255"
  local bland = "\255\211\219\255"
  local mSub, eSub
  local tooltip = Spring.GetCurrentTooltip()
  tooltip, mSub = string.gsub(tooltip, bland.."Me", "\255\1\255\255Me")
  tooltip, eSub = string.gsub(tooltip, bland.." E", "\255\255\255\1   E")
--[[
  tooltip = string.gsub(tooltip, "Experience(.*)Cost(.*)Range([^\n]*)",
                        "Experience"..white.."%1"..bland..
                        "Cost"      ..white.."%2"..bland..
                        "Range"     ..white.."%3"..bland)
]]
  tooltip = string.gsub(tooltip, "Hotkeys:", "\255\255\128\128Hotkeys:\255\128\128\255")
  tooptip =       string.gsub(tooltip, "a", "b")
  local unitTip = ((mSub + eSub) == 2)
--  local _,count = string.gsub(tooltip, "\n", "")
  local i = 0
  for line in string.gfind(tooltip, "([^\n]*)\n?") do
    if (unitTip) then
      if (i == 0) then
        line = "\255\255\128\255" .. line
--        line = "\255\255\128\2" .. line
      else
        line = "\255\200\200\200" .. line
      end
    end  
    gl.Text(line, ySpace * 2, ySpace + (4 - i) * yStep, fontSize, "o")
    i = i + 1
  end
end
