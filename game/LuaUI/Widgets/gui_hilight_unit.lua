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
  local grey   = "\255\192\192\192"
  local yellow = "\255\255\255\128"
  return {
    name      = "HighlightUnit",
    desc      = "Highlights the unit or feature under the cursor\n"..
                grey.."Hold "..yellow.."META"..grey..
                " to show the unit or feature name",
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


local showName = (1 > 0)

local customTex = LUAUI_DIRNAME .. 'Images/highlight_strip.png'
local texName = LUAUI_DIRNAME .. 'Images/highlight_strip.png'
--local texName = 'bitmaps/laserfalloff.tga'

local cylDivs = 64
local cylList = 0

local outlineWidth = 3

local vsx, vsy = widgetHandler:GetViewSizes()
function widget:ViewResize(viewSizeX, viewSizeY)
  vsx = viewSizeX
  vsy = viewSizeY
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:Initialize()
  cylList = gl.CreateList(DrawCylinder, cylDivs)
end


function widget:Shutdown()
  gl.DeleteList(cylList)
  gl.DeleteTexture(customTex)
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

local function HilightModel(drawFunc, drawData, outline)
  gl.DepthTest(true)
  gl.PolygonOffset(-2, -2)
  gl.Blending(GL.SRC_ALPHA, GL.ONE)

  local scale = 20
  local shift = (2 * widgetHandler:GetHourTimer()) % scale
  gl.TexCoord(0, 0)
  gl.TexGen(GL.T, GL.TEXTURE_GEN_MODE, GL.EYE_LINEAR)
  gl.TexGen(GL.T, GL.EYE_PLANE, 0, (1 / scale), 0, shift)
  gl.Texture(texName)

  drawFunc(drawData)

  gl.Texture(false)
  gl.TexGen(GL.T, false)

  -- more edge highlighting
  if (outline) then
    gl.LineWidth(outlineWidth)
    gl.PointSize(outlineWidth)
    gl.PolygonOffset(10, 100)
    gl.PolygonMode(GL.FRONT_AND_BACK, GL.POINT)
    drawFunc(drawData)
    gl.PolygonMode(GL.FRONT_AND_BACK, GL.LINE)
    drawFunc(drawData)
    gl.PolygonMode(GL.FRONT_AND_BACK, GL.FILL)
    gl.PointSize(1)
    gl.LineWidth(1)
  end

  gl.Blending(GL.SRC_ALPHA, GL.ONE_MINUS_SRC_ALPHA)
  gl.PolygonOffset(false)
  gl.DepthTest(false)
end


--------------------------------------------------------------------------------

local function SetUnitColor(unitID, alpha)
  local teamID = Spring.GetUnitTeam(unitID)
  if (teamID == nil) then
    gl.Color(1.0, 0.0, 0.0, alpha) -- red
  elseif (teamID == Spring.GetMyTeamID()) then
    gl.Color(0.0, 1.0, 1.0, alpha) -- cyan
  elseif (Spring.GetUnitAllyTeam(unitID) == Spring.GetMyAllyTeamID()) then
    gl.Color(0.0, 1.0, 0.0, alpha) -- green
  else
    gl.Color(1.0, 0.0, 0.0, alpha) -- red
  end
end


local function SetFeatureColor(featureID, alpha)
  gl.Color(1.0, 0.0, 1.0, alpha) -- purple
  do return end  -- FIXME -- wait for feature team/allyteam resolution

  local allyTeamID = Spring.GetFeatureAllyTeam(featureID)
  if ((allyTeamID == nil) or (allyTeamID < 0)) then
    gl.Color(1.0, 1.0, 1.0, alpha) -- white
  elseif (allyTeamID == Spring.GetMyAllyTeamID()) then
    gl.Color(0.0, 1.0, 1.0, alpha) -- cyan
  else
    gl.Color(1.0, 0.0, 0.0, alpha) -- red
  end
end


local function UnitDrawFunc(unitID)
  gl.Unit(unitID, true)
end


local function FeatureDrawFunc(featureID)
  gl.Feature(featureID, true)
end


local function HilightUnit(unitID)
  local outline = (Spring.GetUnitIsCloaked(unitID) ~= true)
  SetUnitColor(unitID, outline and 0.5 or 0.25)
  HilightModel(UnitDrawFunc, unitID, outline)
end


local function HilightFeatureModel(featureID)
  SetFeatureColor(featureID, 0.5)
  HilightModel(FeatureDrawFunc, featureID, true)
end


local function HilightFeature(featureID)
  local fDefID = Spring.GetFeatureDefID(featureID)
  local fd = FeatureDefs[fDefID]
  if (fd == nil) then return end

  if (fd.drawType == 0) then
    HilightFeatureModel(featureID)
    return
  end

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


function widget:DrawScreen()
  local a,c,m,s = Spring.GetModKeyState()
  if (not m) then
    return
  end

  local mx, my = Spring.GetMouseState()
  local type, data = Spring.TraceScreenRay(mx, my)

  local str = ''

  local cheat  = Spring.IsCheatingEnabled()

  if (type == 'unit') then
    local udid = Spring.GetUnitDefID(data)
    if (udid == nil) then return end
    local ud = UnitDefs[udid]
    if (ud == nil) then return end
    str = YellowStr .. ud.humanName -- .. ' ' .. CyanStr .. ud.tooltip
    if (cheat) then
      str = str .. ' ' .. '\255\255\255\255#' .. data
    end
  elseif (type == 'feature') then
    local fdid = Spring.GetFeatureDefID(data)
    if (fdid == nil) then return end
    local fd = FeatureDefs[fdid]
    if (fd == nil) then return end
    str = '\255\255\128\255' .. fd.tooltip
    if (cheat) then
      str = str .. ' ' .. '\255\255\255\255#' .. data
    end
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
              

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
