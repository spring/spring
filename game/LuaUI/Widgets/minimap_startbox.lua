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

if (Game.startPosType ~= 2) then
  return false
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local gl = gl  --  use a local copy for faster access


-- map sizes
local msx = Game.mapX * 512.0
local msz = Game.mapY * 512.0

local xformList = 0
local coneList = 0

local startTimer = Spring.GetTimer()

local gaiaTeamID
local gaiaAllyTeamID



--------------------------------------------------------------------------------

function widget:Initialize()
  -- only show at the beginning
  if (Spring.GetGameFrame() > 1) then
    widgetHandler:RemoveWidget()
    return
  end

  gaiaTeamID = Spring.GetGaiaTeamID()
  if (gaiaTeamID) then
    local _,_,_,_,_,_,_,_,_,_,atid = Spring.GetTeamInfo(gaiaTeamID)
    gaiaAllyTeamID = atid
  end

  -- flip and scale  (using x & y for gl.Rect())
  xformList = gl.CreateList(function()
    gl.LoadIdentity()
    gl.Translate(0, 1, 0)
    gl.Scale(1 / msx, -1 / msz, 1)
  end)

  coneList = gl.CreateList(function()
    local h = 100
    local r = 25
    local divs = 32
    gl.BeginEnd(GL.TRIANGLE_FAN, function()
      gl.Vertex( 0, h,  0)
      for i = 0, divs do
        local a = i * ((math.pi * 2) / divs)
        local cosval = math.cos(a)
        local sinval = math.sin(a)
        gl.Vertex(r * sinval, 0, r * cosval)
      end
    end)
  end)
end


function widget:Shutdown()
  gl.DeleteList(xformList)
  gl.DeleteList(coneList)
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local teamColors = {}


local function GetTeamColor(teamID)
  local color = teamColors[teamID]
  if (color) then
    return color
  end
  local _,_,_,_,_,_,r,g,b = Spring.GetTeamInfo(teamID)
  
  color = { r, g, b }
  teamColors[teamID] = color
  return color
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local teamColorStrs = {}


local function GetTeamColorStr(teamID)
  local colorSet = teamColorStrs[teamID]
  if (colorSet) then
    return colorSet[1], colorSet[2]
  end

  local outlineChar = ''
  local _,_,_,_,_,_,r,g,b = Spring.GetTeamInfo(teamID)
  if (r and g and b) then
    local function ColorChar(x)
      local c = math.floor(x * 255)
      c = ((c <= 1) and 1) or ((c >= 255) and 255) or c
      return string.char(c)
    end
    local colorStr
    colorStr = '\255'
    colorStr = colorStr .. ColorChar(r)
    colorStr = colorStr .. ColorChar(g)
    colorStr = colorStr .. ColorChar(b)
    local i = (r * 0.299) + (g * 0.587) + (b * 0.114)
    outlineChar = ((i > 0.25) and 'o') or 'O'
    teamColorStrs[teamID] = { colorStr, outlineChar }
    return colorStr, outlineChar
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:DrawWorld()
  -- show the team start positions
  gl.Fog(false)
  for _, teamID in ipairs(Spring.GetTeamList()) do
    if (teamID ~= gaiaTeamID) then
      local _,leader = Spring.GetTeamInfo(teamID)
      local name = Spring.GetPlayerInfo(leader)
      local x, y, z = Spring.GetTeamStartPosition(teamID)
      if (x) then
        local color = GetTeamColor(teamID)
        local time = Spring.DiffTimers(Spring.GetTimer(), startTimer)
        local alpha = 0.5 + math.abs(((time * 3) % 1) - 0.5)
        gl.PushMatrix()
        gl.Translate(x, y, z)
        gl.Color(color[1], color[2], color[3], alpha)
        gl.CallList(coneList)
        gl.PopMatrix()
      end
    end
  end
  gl.Fog(true)
end


function widget:DrawScreen()
  -- show the team start positions
  gl.Fog(false)
  for _, teamID in ipairs(Spring.GetTeamList()) do
    if (teamID ~= gaiaTeamID) then
      local _,leader = Spring.GetTeamInfo(teamID)
      local name = Spring.GetPlayerInfo(leader)
      local colorStr, outlineStr = GetTeamColorStr(teamID)
      name = colorStr .. name
      local x, y, z = Spring.GetTeamStartPosition(teamID)
      if (x) then
        local sx, sy, sz = Spring.WorldToScreenCoords(x, y + 135, z)
        if (sz < 1) then
          gl.Text(name, sx, sy + 12, 12, outlineStr .. 'c')
        end
      end
    end
  end
  gl.Fog(true)
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:DrawInMiniMap(sx, sz)
  -- only show at the beginning
  if (Spring.GetGameFrame() > 1) then
    widgetHandler:RemoveWidget()
  end

  gl.PushMatrix()
  gl.CallList(xformList)

  gl.LineWidth(1.49)

  local gaiaAllyTeamID
  local gaiaTeamID = Spring.GetGaiaTeamID()
  if (gaiaTeamID) then
    local _,_,_,_,_,_,_,_,_,_,atid = Spring.GetTeamInfo(gaiaTeamID)
    gaiaAllyTeamID = atid
  end

  -- show all start boxes
  for _,at in ipairs(Spring.GetAllyTeamList()) do
    if (at ~= gaiaAllyTeamID) then
      local xn, zn, xp, zp = Spring.GetAllyTeamStartBox(at)
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
  end

  gl.LineWidth(1.0)

  -- show the team start positions
  for _, teamID in ipairs(Spring.GetTeamList()) do
    if (teamID ~= gaiaTeamID) then
      local x, y, z = Spring.GetTeamStartPosition(teamID)
      if (x) then
        local color = GetTeamColor(teamID)
        local time = Spring.DiffTimers(Spring.GetTimer(), startTimer)
        local alpha = 0.5 + math.abs(((time * 3) % 1) - 0.5)
        gl.PointSize(6.5)
        gl.Color(0, 0, 0)
        gl.BeginEnd(GL.POINTS, function() gl.Vertex(x, z) end)
        gl.PointSize(5.0)
        gl.Color(color[1], color[2], color[3], alpha)
        gl.BeginEnd(GL.POINTS, function() gl.Vertex(x, z) end)
      end
    end
  end
  gl.PointSize(1.0)

  gl.PopMatrix()
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
