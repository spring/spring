--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    gui_minimap.lua
--  brief:   a demo widget to show how the minimap can be controlled
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "Minimap",
    desc      = "Minimap redirection -- does nothing  (for devs)",
    author    = "trepan",
    date      = "Jan 8, 2007",
    license   = "GNU GPL, v2 or later",
    drawLayer = 1,
    enabled   = false  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

include("colors.h.lua")
include("opengl.h.lua")

local xsize = 300
local ysize = 300
local minx = 10
local miny = 10
local maxx = xsize + minx - 1
local maxy = ysize + miny - 1

local oldGeo = Spring.GetConfigString("MiniMapGeometry", "2 2 200 200")


function widget:Initialize()
  Spring.SendCommands({
    "minimap slavemode 1",
    string.format("minimap geometry %i %i %i %i", minx, miny, xsize, ysize)
  })
end


function widget:Shutdown()
  Spring.SendCommands({
    "minimap slavemode 0",
    "minimap geometry " .. oldGeo
  })
end


local vsx, vsy = widgetHandler:GetViewSizes()
function widget:ViewResize(viewSizeX, viewSizeY)
  vsx = viewSizeX
  vsy = viewSizeY
end


function widget:DrawScreen()
  if (widgetHandler:InTweakMode()) then
    Spring.SendCommands({
      "minimap min 1",
      "minimap draw"
    })
  else
    Spring.SendCommands({
      "minimap min 0",
      "minimap draw"
    })
  end
  
  local xn = minx - 0.5
  local xp = maxx + 0.5
  local yn = vsy - (miny - 0.5)
  local yp = vsy - (maxy + 0.5)

  gl.Color(1, 1, 1)
  gl.Shape(GL_LINE_LOOP, {
    { v = { xn, yn } }, { v = { xp, yn } },
    { v = { xp, yp } }, { v = { xn, yp } }
  })
  gl.Color(1, 1, 1)
  xn = xn - 1
  xp = xp + 1
  yn = yn + 1
  yp = yp - 1
  gl.Color(0, 0, 0)
  gl.Shape(GL_LINE_LOOP, {
    { v = { xn, yn } }, { v = { xp, yn } },
    { v = { xp, yp } }, { v = { xn, yp } }
  })
  
end
