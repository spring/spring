--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    gui_comm_ends.lua
--  brief:   shows a pre-game warning if commander-ends is enabled
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "CommanderEnds",
    desc      = "Indicator for the CommEnds state (at game start)",
    author    = "trepan",
    date      = "Jan 11, 2007",
    license   = "GNU GPL, v2 or later",
    drawLayer = 3,
    enabled   = true  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

include("opengl.h.lua")
include("colors.h.lua")


local vsx, vsy = widgetHandler:GetViewSizes()

function widget:ViewResize(viewSizeX, viewSizeY)
  vsx = viewSizeX
  vsy = viewSizeY
end


function widget:DrawScreen()
  if (Spring.GetGameSeconds() > 1) then
    widgetHandler:RemoveWidget()
  end
  if (Game.commEnds) then
    local timer = widgetHandler:GetHourTimer()
    local colorStr
    if (math.mod(timer, 0.5) < 0.25) then
      colorStr = RedStr
    else
      colorStr = YellowStr
    end
    gl.Text(colorStr .. "Commander Ends Game!!!",
            (vsx * 0.5), (vsy * 0.5) - 50, 24, "oc")
  end
end

