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
    date      = "Jan 8, 2007",
    license   = "GNU GPL, v2 or later",
    layer     = 0,
    enabled   = false  --  loaded by default?
  }
end

-------------------------------------------------------------------------------
-------------------------------------------------------------------------------

include("colors.h.lua")


local vsx, vsy = widgetHandler:GetViewSizes()

function widget:ViewResize(viewSizeX, viewSizeY)
  vsx = viewSizeX
  vsy = viewSizeY
end


-------------------------------------------------------------------------------

local GL_LINES           = GL.LINES
local glBeginEnd         = gl.BeginEnd
local glColor            = gl.Color
local glVertex           = gl.Vertex
local spGetActiveCommand = Spring.GetActiveCommand
local spGetMouseState    = Spring.GetMouseState


-------------------------------------------------------------------------------

centerGap = 20

centerColors = {
  [CMD.AREA_ATTACK] = { 1.0, 0.0, 0.0, 1.0 },
  [CMD.ATTACK]      = { 1.0, 0.0, 0.0, 1.0 },
  [CMD.DGUN]        = { 1.0, 0.0, 0.0, 1.0 },
  [CMD.FIGHT]       = { 1.0, 0.0, 0.0, 1.0 },
  [CMD.MOVE]        = { 0.0, 1.0, 0.0, 1.0 },
  [CMD.GUARD]       = { 0.0, 0.0, 1.0, 1.0 },
  [CMD.PATROL]      = { 0.0, 0.0, 1.0, 1.0 },
  [CMD.CAPTURE]     = { 1.0, 1.0, 0.0, 1.0 },
  [CMD.REPAIR]      = { 0.0, 1.0, 1.0, 1.0 },
  [CMD.RECLAIM]     = { 1.0, 0.0, 1.0, 1.0 },
  [CMD.RESURRECT]   = { 0.6, 1.0, 0.6, 1.0 },
  [CMD.RESTORE]     = { 0.0, 1.0, 0.0, 1.0 },
}

edgeColors = {
  [CMD.AREA_ATTACK] = { 0.0, 0.0, 0.0, 1.0 },
--[CMD.ATTACK]      = { 1.0, 0.0, 0.0, 1.0 },
  [CMD.DGUN]        = { 1.0, 1.0, 0.0, 1.0 },
  [CMD.FIGHT]       = { 0.0, 1.0, 0.0, 1.0 },
--[CMD.MOVE]        = { 0.0, 1.0, 0.0, 1.0 },
--[CMD.GUARD]       = { 0.0, 0.0, 1.0, 1.0 },
  [CMD.PATROL]      = { 0.0, 1.0, 0.5, 1.0 },
--[CMD.CAPTURE]     = { 1.0, 1.0, 0.0, 1.0 },
  [CMD.REPAIR]      = { 0.0, 1.0, 0.0, 1.0 },
  [CMD.RECLAIM]     = { 0.0, 1.0, 0.0, 1.0 },
  [CMD.RESTORE]     = { 0.0, 1.0, 0.0, 1.0 },
  [CMD.RESURRECT]   = { 0.0, 1.0, 0.0, 1.0 },
}


-------------------------------------------------------------------------------

function widget:DrawScreen()
  mx, my = spGetMouseState()
  mx = mx + 0.5
  my = my + 0.5

  local cc = { 1.0, 1.0, 1.0, 1.0 } -- center color
  local ec = { 0.5, 0.5, 0.5, 1.0 } -- edge   color
  
  local index, id, type, name = spGetActiveCommand()
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

  gl.BeginEnd(GL_LINES, function()
    glColor(cc); glVertex(mx, my - gap); glColor(ec); glVertex(mx,   0)
    glColor(cc); glVertex(mx, my + gap); glColor(ec); glVertex(mx, vsy)
    glColor(cc); glVertex(mx - gap, my); glColor(ec); glVertex(0,   my)
    glColor(cc); glVertex(mx + gap, my); glColor(ec); glVertex(vsx, my)
  end)
end


-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
