--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    gui_highlight_unit.lua
--  brief:   highlights the unit/feature under the cursor
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "HighlightUnit",
    desc      = "Highlights the unit or feature under the cursor",
    author    = "trepan",
    date      = "Apr 16, 2007",
    license   = "GNU GPL, v2 or later",
    layer     = 5,
    enabled   = false  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

include("colors.h.lua")


local cylDivs = 64
local cylList = 0

local blink = (-1 > 0)

local showName = (-1 > 0)

local vsx, vsy = widgetHandler:GetViewSizes()
if (showName) then
  function widget:ViewResize(viewSizeX, viewSizeY)
    vsx = viewSizeX
    vsy = viewSizeY
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:Initialize()
  cylList = gl.CreateList(DrawCylinder, cylDivs)
end


function widget:Shutdown()
  gl.DeleteList(cylList)
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function DrawCylinder(divs)
  local cos = math.cos
  local sin = math.sin
  local divRads = (2.0 * math.pi) / divs
  -- top
  gl.BeginEnd(GL.TRIANGLE_FAN, function()
    for i = 1, divs do
      local a = i * divRads
      gl.Vertex(sin(a), 1.0, cos(a))
    end
  end)
  -- bottom
  gl.BeginEnd(GL.TRIANGLE_FAN, function()
    for i = 1, divs do
      local a = -i * divRads
      gl.Vertex(sin(a), -1.0, cos(a))
    end
  end)
  -- sides
  gl.BeginEnd(GL.QUAD_STRIP, function()
    for i = 0, divs do
      local a = i * divRads
      gl.Vertex(sin(a),  1.0, cos(a))
      gl.Vertex(sin(a), -1.0, cos(a))
    end
  end)
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function SetUnitColorMask(unitID)
  local team = Spring.GetUnitTeam(unitID)
  if (not team) then
    gl.ColorMask(true,  false, false, false) -- red
  elseif (team == Spring.GetMyTeamID()) then
    gl.ColorMask(false, true,  true,  false) -- cyan
  elseif (Spring.GetUnitAllyTeam(unitID) == Spring.GetMyAllyTeamID()) then
    gl.ColorMask(false, true,  false, false) -- green
  else
    gl.ColorMask(true,  false, false, false) -- red
  end
end


local function HilightUnit(unitID)
  gl.DepthTest(true)
  gl.Color(1, 0, 0, 0.25)

  gl.LogicOp(GL.SET)
  SetUnitColorMask(unitID)
  gl.Unit(unitID)
  gl.ColorMask(true, true, true, true)
  gl.LogicOp(false)

  gl.DepthTest(false)
end


local function HilightFeature(featureID)
  local fDefID = Spring.GetFeatureDefID(featureID)

  local fd = FeatureDefs[fDefID]
  if (fd == nil) then return end
  local radius = fd.radius

  local px, py, pz = Spring.GetFeaturePosition(featureID)
  if (px == nil) then return end

  local yScale = 4
  gl.PushMatrix()
  gl.Translate(px, py, pz)
  gl.Scale(radius, yScale * radius, radius)
  -- FIXME: needs an 'inside' check

  gl.DepthTest(true)
  gl.LogicOp(GL.INVERT)

  gl.Culling(GL.FRONT)
  gl.CallList(cylList)

  gl.Culling(GL.BACK)
  gl.CallList(cylList)

  gl.LogicOp(false)
  gl.Culling(false)
  gl.DepthTest(false)

  gl.PopMatrix()
end


local GetMyPlayerID           = Spring.GetMyPlayerID
local GetPlayerControlledUnit = Spring.GetPlayerControlledUnit


function widget:DrawWorld()
  if (blink and (math.mod(widgetHandler:GetHourTimer(), 0.1) > 0.05)) then
    return
  end

  local mx, my = Spring.GetMouseState()
  local type, data = Spring.TraceScreenRay(mx, my)

  if (type == 'feature') then
    HilightFeature(data)
  elseif (type == 'unit') then
    local unitID = GetPlayerControlledUnit(GetMyPlayerID())
    if (data ~= unitID) then
      HilightUnit(data)
    end
  end
end
              

if (showName) then
  function widget:DrawScreen()
    if (blink and (math.mod(widgetHandler:GetHourTimer(), 0.1) > 0.05)) then
      return
    end

    local mx, my = Spring.GetMouseState()
    local type, data = Spring.TraceScreenRay(mx, my)

    local str = ''

    if (type == 'unit') then
      local udid = Spring.GetUnitDefID(data)
      if (udid == nil) then return end
      local ud = UnitDefs[udid]
      if (ud == nil) then return end
      str = YellowStr .. ud.humanName -- .. ' ' .. CyanStr .. ud.tooltip
    elseif (type == 'feature') then
      local fdid = Spring.GetFeatureDefID(data)
      if (fdid == nil) then return end
      local fd = FeatureDefs[fdid]
      if (fd == nil) then return end
      str = '\255\255\128\255' .. fd.tooltip
    end

    local f = 14
    local g = 10
    local l = f * gl.GetTextWidth(str)
    if ((mx + l + g) < vsx) then
      gl.Text(str, mx + g, my + g, f, 'o')
    else
      gl.Text(str, mx - g, my + g, f, 'or')
    end
  end
end
              

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
