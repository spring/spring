--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    gui_team_platter.lua
--  brief:   team colored platter for all visible units
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "TeamPlatter",
    desc      = "Shows a team color platter above all visible units",
    author    = "trepan",
    date      = "Apr 16, 2007",
    license   = "GNU GPL, v2 or later",
    layer     = 5,
    enabled   = false  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function SetupCommandColors(state)
  local alpha = state and 1 or 0
  local f = io.open('cmdcolors.tmp', 'w+')
  if (f) then
    f:write('unitBox  0 1 0 ' .. alpha)
    f:close()
    Spring.SendCommands({'cmdcolors cmdcolors.tmp'})
  end
  os.remove('cmdcolors.tmp')
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local teamColors = {}

local trackSlope = true

local circleBoth   = 0
local circleLines  = 0
local circlePolys  = 0
local circleDivs   = 32
local circleOffset = 0

local startTimer = Spring.GetTimer()

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:Initialize()
  circleBoth = gl.CreateList(function()
    gl.BeginEnd(GL.TRIANGLE_FAN, function()
      local radstep = (2.0 * math.pi) / circleDivs
      for i = 1, circleDivs do
        local a = (i * radstep)
        gl.Vertex(math.sin(a), circleOffset, math.cos(a))
      end
    end)
    gl.BeginEnd(GL.LINE_LOOP, function()
      local radstep = (2.0 * math.pi) / circleDivs
      for i = 1, circleDivs do
        local a = (i * radstep)
        gl.Vertex(math.sin(a), circleOffset, math.cos(a))
      end
    end)
  end)

  circleLines = gl.CreateList(function()
    gl.BeginEnd(GL.LINE_LOOP, function()
      local radstep = (2.0 * math.pi) / circleDivs
      for i = 1, circleDivs do
        local a = (i * radstep)
        gl.Vertex(math.sin(a), circleOffset, math.cos(a))
      end
    end)
  end)

  circlePolys = gl.CreateList(function()
    gl.BeginEnd(GL.TRIANGLE_FAN, function()
      local radstep = (2.0 * math.pi) / circleDivs
      for i = 1, circleDivs do
        local a = (i * radstep)
        gl.Vertex(math.sin(a), circleOffset, math.cos(a))
      end
    end)
  end)

  SetupCommandColors(false)
end


function widget:Shutdown()
  gl.DeleteList(circleBoth)
  gl.DeleteList(circleLines)
  gl.DeleteList(circlePolys)

  SetupCommandColors(true)
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

--
-- Speed-ups
--

local GetUnitTeam          = Spring.GetUnitTeam
local GetUnitRadius        = Spring.GetUnitRadius
local GetUnitDefID         = Spring.GetUnitDefID
local GetUnitViewPosition  = Spring.GetUnitViewPosition
local GetUnitBasePosition  = Spring.GetUnitBasePosition
local IsUnitVisible        = Spring.IsUnitVisible
local IsUnitSelected       = Spring.IsUnitSelected
local GetGroundNormal      = Spring.GetGroundNormal
local GetUnitDefDimensions = Spring.GetUnitDefDimensions

local glColor          = gl.Color
local glDrawListAtUnit = gl.DrawListAtUnit

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local realRadii = {}

local function GetUnitRealRadius(unitID)
  local udid = GetUnitDefID(unitID)
  local radius = realRadii[udid]
  if (radius) then
    return radius
  end

  local ud = UnitDefs[GetUnitDefID(unitID)]
  if (ud == nil) then return nil end

  local dims = GetUnitDefDimensions(udid)
  if (dims == nil) then return nil end

  local scale = ud.hitSphereScale
  scale = (scale == 0.0) and 1.0 or scale
  radius = dims.radius / scale
  realRadii[udid] = radius
  return radius
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:DrawWorldPreUnit()
  gl.LineWidth(3.0)

  gl.DepthTest(true)
  
--  gl.Texture(false)
--  gl.AlphaTest(false)

  gl.PolygonOffset(-50, 10)

  local lastColorSet = nil
  for _,unitID in ipairs(Spring.GetAllUnits()) do
    if (IsUnitVisible(unitID)) then
      local teamID = GetUnitTeam(unitID)
      if (teamID) then
        local radius = GetUnitRealRadius(unitID)--GetUnitRadius(unitID)
        if (radius) then
          local colorSet  = teamColors[teamID]
          if (trackSlope) then
            local x, y, z = GetUnitBasePosition(unitID)
            local gx, gy, gz = GetGroundNormal(x, z)
            local degrot = math.acos(gy) * 180 / math.pi
            if (colorSet) then
              glColor(colorSet[1])
              glDrawListAtUnit(unitID, circleBoth, false,
                               radius, 1.0, radius,
                               degrot, gz, 0, -gx)
              glColor(colorSet[2])
              glDrawListAtUnit(unitID, circleLines, false,
                               radius, 1.0, radius,
                               degrot, gz, 0, -gx)
            else
              local _,_,_,_,_,_,r,g,b = Spring.GetTeamInfo(teamID)
              teamColors[teamID] = {{ r, g, b, 0.4 },
                                    { r, g, b, 0.5 }}
            end
          else
            if (colorSet) then
              glColor(colorSet[1])
              glDrawListAtUnit(unitID, circleBoth, false,
                               radius, 1.0, radius)
              glColor(colorSet[2])
              glDrawListAtUnit(unitID, circleLines, false,
                               radius, 1.0, radius)
            else
              local _,_,_,_,_,_,r,g,b = Spring.GetTeamInfo(teamID)
              teamColors[teamID] = {{ r, g, b, 0.4 },
                                    { r, g, b, 0.5 }}
            end
          end
        end
      end
    end
  end

  gl.PolygonOffset(false)

  --
  -- Blink the selected units
  --

  gl.DepthTest(false)

  local diffTime = Spring.DiffTimers(Spring.GetTimer(), startTimer)
  local alpha = 1.8 * math.abs(0.5 - (diffTime * 3.0 % 1.0))
  gl.Color(1, 1, 1, alpha)

  for _,unitID in ipairs(Spring.GetSelectedUnits()) do
    local radius = GetUnitRadius(unitID)
    if (radius) then
      if (trackSlope) then
        local x, y, z = GetUnitBasePosition(unitID)
        local gx, gy, gz = GetGroundNormal(x, z)
        local degrot = math.acos(gy) * 180 / math.pi
        glDrawListAtUnit(unitID, circleLines, false,
                         radius, 1.0, radius,
                          degrot, gz, 0, -gx)
      else
        glDrawListAtUnit(unitID, circleLines, false,
                         radius, 1.0, radius)
      end
    end
  end

  gl.DepthTest(false)

  gl.LineWidth(1.0)
end
              

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
