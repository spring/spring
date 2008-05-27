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
    date      = "Jan 8, 2007",
    license   = "GNU GPL, v2 or later",
    layer     = -3,
    enabled   = false  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

-- Automatically generated local definitions

local glPopMatrix      = gl.PopMatrix
local glPushMatrix     = gl.PushMatrix
local glRotate         = gl.Rotate
local glScale          = gl.Scale
local glText           = gl.Text
local glTranslate      = gl.Translate
local spGetGameSeconds = Spring.GetGameSeconds


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

include("colors.h.lua")

local floor = math.floor


local font = 'FreeMonoBold'
local fontSize = 32
local fontName = LUAUI_DIRNAME..'Fonts/'..font..'_'..fontSize

local fh = fontHandler.UseFont(fontName)

local vsx, vsy = widgetHandler:GetViewSizes()
function widget:ViewResize(viewSizeX, viewSizeY)
  vsx = viewSizeX
  vsy = viewSizeY
end


function widget:DrawScreen()
  if (spGetGameSeconds() > 1) then
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

    local msg = colorStr .. "Commander Ends Game!!!"
    glPushMatrix()
    glTranslate((vsx * 0.5), (vsy * 0.5) - 50, 0)
    glScale(1.5, 1.5, 1)
    glRotate(30 * math.sin(math.pi * 0.5 * timer), 0, 0, 1)
    if (fh) then
      fh = fontHandler.UseFont(fontName)
      fontHandler.DrawCentered(msg)
    else
      glText(msg, 0, 0, 24, "oc")
    end
    glPopMatrix()
  end
end

