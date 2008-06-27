--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    gui_mousetrail.lua
--  brief:   displays an annoying mouse trail
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "MouseTrail",
    desc      = "Mouse trail toy",
    author    = "trepan",
    date      = "Jan 8, 2007",
    license   = "GNU GPL, v2 or later",
    layer     = 0,
    enabled   = false  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
-- Useless mouse trails
--

local gl = gl  --  use a local copy for faster access

local lifeTime = 1.0
local head = nil  --  a linked list of timestamped vertices
local timer = 0
local mxOld, myOld = Spring.GetMouseState()


function widget:DrawScreen()
  -- adjust timer
  local timer = widgetHandler:GetHourTimer()
  
  -- check for current mouse position
  local mx,my,lmb,mmb,rmb = Spring.GetMouseState()
  if ((mx ~= mxOld) or (my ~= myOld)) then
    -- add a new point  (this defines the node format)
    if (head == nil) then
      head = { head, mxOld, myOld, timer + lifeTime - 0.001 }
    end
    head = { head, mx, my, timer + lifeTime - 0.001 }
    mxOld, myOld = mx, my
  end
  
  -- collect the active vertices (and cull the old ones)
  local elements = {}
  local h = head
  while h do
    local timeLeft = ((h[4] - timer) + 3600.0) % 3600.0
    if (timeLeft > lifeTime) then
      if (h == head) then
        head = nil
      end
      h[1] = nil
    else
      local tf = (timeLeft / lifeTime)
      local ntf = (1.0 - tf)
      table.insert(elements, {
        v = { h[2], h[3] },
        c = { tf, 0.5 * ntf, ntf, tf }
      })
    end
    h = h[1]  -- next node
  end
  
  -- draw the lines
  gl.LineStipple(2, 4095)
  gl.LineWidth(2.0)
  gl.Shape(GL.LINE_STRIP, elements)
  gl.LineWidth(1.0)
  gl.LineStipple(false)
end
