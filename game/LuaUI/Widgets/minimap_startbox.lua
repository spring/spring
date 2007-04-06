--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    minimap_startbox.lua
--  brief:   shows the startboxes in the minimap
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "MiniMap Start Boxes",
    desc      = "MiniMap Start Boxes",
    author    = "trepan",
    date      = "Mar 17, 2007",
    license   = "GNU GPL, v2 or later",
    layer     = 0,
    enabled   = true  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local gl = gl  --  use a local copy for faster access


-- map sizes
local msx = Game.mapX * 512.0
local msz = Game.mapY * 512.0

local xformList = 0


--------------------------------------------------------------------------------

function widget:Initialize()
  -- only show at the beginning
  if (Spring.GetGameFrame() > 1) then
    widgetHandler:RemoveWidget()
    return
  end
  -- flip and scale  (using x & y for gl.Rect())
  xformList = gl.ListCreate(function()
    gl.LoadIdentity()
    gl.Translate(0, 1, 0)
    gl.Scale(1 / msx, -1 / msz, 1)
  end)
end


function widget:Shutdown()
  gl.ListDelete(xformList)
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:DrawInMiniMap(sx, sz)
  -- only show at the beginning
  if (Spring.GetGameFrame() > 1) then
    widgetHandler:RemoveWidget()
  end

  gl.PushMatrix()
  gl.ListRun(xformList)

  gl.LineWidth(1.49)

  -- show all start boxes
  for _,at in ipairs(Spring.GetAllyTeamList()) do
    local xn,zn,xp,zp = Spring.GetAllyTeamStartBox(at)
    if (xn and ((xn ~= 0) or (zn ~= 0) or (xp ~= 1) or (zp ~= 1))) then
      local color
      if (at == Spring.GetMyAllyTeamID()) then
        color = { 0, 1, 0, 0.1 }  --  green
      else
        color = { 1, 0, 0, 0.1 }  --  red
      end
      gl.Color(color)
      gl.Rect(xn, zn, xp, zp)
      color[4] = 0.5  --  pump up the volume
      gl.Color(color)
      gl.PolygonMode(GL.FRONT_AND_BACK, GL.LINE)
      gl.Rect(xn, zn, xp, zp)
      gl.PolygonMode(GL.FRONT_AND_BACK, GL.FILL)
    end
  end

  gl.LineWidth(1.0)

  gl.PopMatrix()
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
