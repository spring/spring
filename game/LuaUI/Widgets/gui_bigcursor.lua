--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    gui_bigcursor.lua
--  brief:   displays a full-screen CAD style cursor
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "BigCursor",
    desc      = "Displays a fullscreen cursor",
    author    = "trepan",
    date      = "Jan 11, 2007",
    license   = "GNU GPL, v2 or later",
    drawLayer = 3,
    enabled   = true  --  loaded by default?
  }
end

-------------------------------------------------------------------------------
-------------------------------------------------------------------------------

include("colors.h.lua")
include("opengl.h.lua")
include("spring.h.lua")


centerGap = 20

local vsx, vsy = widgetHandler:GetViewSizes()

function widget:ViewResize(viewSizeX, viewSizeY)
  vsx = viewSizeX
  vsy = viewSizeY
end


centerColors = {
  [CMD_AREA_ATTACK] = { 1.0, 0.0, 0.0, 1.0 },
  [CMD_ATTACK]      = { 1.0, 0.0, 0.0, 1.0 },
  [CMD_DGUN]        = { 1.0, 0.0, 0.0, 1.0 },
  [CMD_FIGHT]       = { 1.0, 0.0, 0.0, 1.0 },
  [CMD_MOVE]        = { 0.0, 1.0, 0.0, 1.0 },
  [CMD_GUARD]       = { 0.0, 0.0, 1.0, 1.0 },
  [CMD_PATROL]      = { 0.0, 0.0, 1.0, 1.0 },
  [CMD_CAPTURE]     = { 1.0, 1.0, 0.0, 1.0 },
  [CMD_REPAIR]      = { 0.0, 1.0, 1.0, 1.0 },
  [CMD_RECLAIM]     = { 1.0, 0.0, 1.0, 1.0 },
  [CMD_RESURRECT]   = { 0.6, 1.0, 0.6, 1.0 },
  [CMD_RESTORE]     = { 0.0, 1.0, 0.0, 1.0 },
}

edgeColors = {
  [CMD_AREA_ATTACK] = { 0.0, 0.0, 0.0, 1.0 },
--[CMD_ATTACK]      = { 1.0, 0.0, 0.0, 1.0 },
  [CMD_DGUN]        = { 1.0, 1.0, 0.0, 1.0 },
  [CMD_FIGHT]       = { 0.0, 1.0, 0.0, 1.0 },
--[CMD_MOVE]        = { 0.0, 1.0, 0.0, 1.0 },
--[CMD_GUARD]       = { 0.0, 0.0, 1.0, 1.0 },
  [CMD_PATROL]      = { 0.0, 1.0, 0.5, 1.0 },
--[CMD_CAPTURE]     = { 1.0, 1.0, 0.0, 1.0 },
  [CMD_REPAIR]      = { 0.0, 1.0, 0.0, 1.0 },
  [CMD_RECLAIM]     = { 0.0, 1.0, 0.0, 1.0 },
  [CMD_RESTORE]     = { 0.0, 1.0, 0.0, 1.0 },
  [CMD_RESURRECT]   = { 0.0, 1.0, 0.0, 1.0 },
}


function widget:DrawScreen()
  mx, my = Spring.GetMouseState()
  mx = mx + 0.5
  my = my + 0.5

  local cc = { 1.0, 1.0, 1.0, 1.0 } -- center color
  local ec = { 0.5, 0.5, 0.5, 1.0 } -- edge   color
  
  local index, id, type, name = Spring.GetActiveCommand()
  if (index and id) then
    if (id < 0) then
      -- build colors
      cc = { 0.0, 1.0, 0.0, 1.0 }
      ec = { 0.0, 0.0, 1.0, 1.0 }
    else
      local ecTmp = edgeColors[id]
      if (ecTmp) then
        ec = ecTmp
      end
      local ccTmp = centerColors[id]
      if (ccTmp) then
        cc = ccTmp
      else
        cc = { 0.0, 0.0, 0.0, 1.0 }
      end

    end
  end
  
  local gap = centerGap

  gl.Shape(GL_LINES, {
    { v = { mx, my - gap }, c = cc }, { v = { mx,   0 }, c = ec },
    { v = { mx, my + gap }, c = cc }, { v = { mx, vsy }, c = ec },
    { v = { mx - gap, my }, c = cc }, { v = {  0,  my }, c = ec },
    { v = { mx + gap, my }, c = cc }, { v = { vsx, my }, c = ec },
  })
end
