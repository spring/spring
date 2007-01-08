--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    gui_selbuttons.lua
--  brief:   adds a selected units button control panel
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "SelectionButtons",
    desc      = "Buttons for the current selection (incomplete)",
    author    = "trepan",
    date      = "Jan 8, 2007",
    license   = "GNU GPL, v2 or later",
    drawLayer = 0,
    enabled   = true  --  loaded by default?
  }
end

-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
--
--  Disabled for Spring versions older then 0.74b3 (no GetUnitDefDimensions())
--

function widget:Initialize()
  if (Spring.GetUnitDefDimensions == nil) then
    Spring.SendCommands({"echo Selection Buttons widget has been disabled"})
    widgetHandler:RemoveWidget()
  end
end


-------------------------------------------------------------------------------
-------------------------------------------------------------------------------

include("colors.h.lua")
include("opengl.h.lua")

local vsx, vsy = widgetHandler:GetViewSizes()

function widget:ViewResize(viewSizeX, viewSizeY)
  vsx = viewSizeX
  vsy = viewSizeY
end


-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
--
--  Selection Icons (rough around the edges)
--

local unitTypes = 0
local countsTable = {}
local activePress = false
local mouseIcon = -1

local iconSizeX = math.floor(100)
local iconSizeY = math.floor(iconSizeX * 0.75)
local fontSize = iconSizeY * 0.25

local rectMinX = 0
local rectMaxX = 0
local rectMinY = 0
local rectMaxY = 0


function widget:DrawScreen()
  unitCounts = Spring.GetSelectedUnitsCounts()
  unitTypes = unitCounts.n;
  if (unitTypes <= 0) then
    countsTable = {}
    activePress = false
    return
  end
  
  SetupDimensions(unitTypes)

  -- unit model rendering uses the depth-buffer
  gl.Clear(GL_DEPTH_BUFFER_BIT)

  -- draw the buildpics
  unitCounts.n = nil  
  icon = 0
  for udid,count in pairs(unitCounts) do
    DrawUnitDefIcon(udid, icon, count)
    icon = icon + 1
  end

  -- draw the highlights
  if (not widgetHandler:InTweakMode()) then
    x,y,lb,mb,rb = Spring.GetMouseState()
    icon = MouseOverIcon(x, y)
    if (icon >= 0) then
      if (lb or mb or rb) then
        DrawIconQuad(icon, { 1, 0, 0, 0.333 })  --  red highlight
      else
        DrawIconQuad(icon, { 0, 0, 1, 0.333 })  --  blue highlight
      end
    end
  end
end


function SetupDimensions(count)
  xmid = vsx * 0.5
  width = math.floor(iconSizeX * count)
  rectMinX = math.floor(xmid - (0.5 * width))
  rectMaxX = math.floor(xmid + (0.5 * width))
  rectMinY = math.floor(0)
  rectMaxY = math.floor(rectMinY + iconSizeY)
end


function CenterUnitDef(unitDefID)
  local ud = UnitDefs[unitDefID] 
  if (not ud) then
    return
  end
  if (not ud.dimensions) then
    ud.dimensions = Spring.GetUnitDefDimensions(unitDefID)
  end
  if (not ud.dimensions) then
    return
  end

  local d = ud.dimensions
  local xSize = (d.maxx - d.minx)
  local ySize = (d.maxy - d.miny)
  local zSize = (d.maxz - d.minz)

  local hSize -- maximum horizontal dimension
  if (xSize > zSize) then hSize = xSize else hSize = zSize end

  -- aspect ratios
  local mAspect = hSize / ySize
  local vAspect = iconSizeX / iconSizeY

  -- scale the unit to the box (maxspect)
  local scale
  if (mAspect > vAspect) then
    scale = (iconSizeX / hSize)
  else
    scale = (iconSizeY / ySize)
  end
  scale = scale * 0.8
  gl.Scale(scale, scale, scale)

  -- translate to the unit's midpoint
  local xMid = 0.5 * (d.maxx + d.minx)
  local yMid = 0.5 * (d.maxy + d.miny)
  local zMid = 0.5 * (d.maxz + d.minz)
  gl.Translate(-xMid, -yMid, -zMid)
end


local function SetupModelDrawing()
  gl.DepthTest(true) 
  gl.DepthMask(true)
  gl.Culling(GL_FRONT)
  gl.Lighting(true)
  gl.Blending(false)
  gl.Material({
    ambient  = { 0.2, 0.2, 0.2, 1.0 },
    diffuse  = { 1.0, 1.0, 1.0, 1.0 },
    emission = { 0.0, 0.0, 0.0, 1.0 },
    specular = { 0.2, 0.2, 0.2, 1.0 },
    shininess = 16.0
  })
end


local function RevertModelDrawing()
  gl.Blending(true)
  gl.Lighting(false)
  gl.Culling(false)
  gl.DepthMask(false)
  gl.DepthTest(false)
end


local function SetupBackgroundColor(ud)
  local alpha = 0.95
  if (ud.canFly) then
    gl.Color(0.5, 0.5, 0.0, alpha)
  elseif (ud.floater) then
    gl.Color(0.0, 0.0, 0.5, alpha)
  elseif (ud.builder) then
    gl.Color(0.0, 0.5, 0.0, alpha)
  else
    gl.Color(.5, .5, .5, alpha)
  end
end


function DrawUnitDefIcon(unitDefID, iconPos, count)
  local xmin = math.floor(rectMinX + (iconSizeX * iconPos))
  local xmax = xmin + iconSizeX
  if ((xmax < 0) or (xmin > vsx)) then return end  -- bail
  
  local ymin = rectMinY
  local ymax = rectMaxY
  local xmid = (xmin + xmax) * 0.5
  local ymid = (ymin + ymax) * 0.5

  local ud = UnitDefs[unitDefID] 

  -- draw background quad
--  gl.Color(0.3, 0.3, 0.3, 1.0)
--  gl.Texture('#'..unitDefID)
  SetupBackgroundColor(ud)
  gl.Shape(GL_QUADS, {
    { v = { xmin + 1, ymin + 1 }, t = { 0, 1 } },
    { v = { xmax - 0, ymin + 1 }, t = { 1, 1 } },
    { v = { xmax - 0, ymax - 0 }, t = { 1, 0 } },
    { v = { xmin + 1, ymax - 0 }, t = { 0, 0 } },
  })
  gl.Texture(false)

  -- draw the 3D unit
	SetupModelDrawing()
  
  gl.PushMatrix()
  gl.Scissor(xmin, ymin, xmax - xmin, ymax - ymin)
  gl.Translate(xmid, ymid, 0)
  gl.Rotate(15.0, 1, 0, 0)
  gl.Rotate(math.cos(0.5 * math.pi * Spring.GetGameSeconds()) * 60.0, 0, 1, 0)

  CenterUnitDef(unitDefID)

--  gl.Lighting(false)
--  gl.Culling(false)
--  gl.Color(0,0,0,1)

  gl.UnitDef(unitDefID, Spring.GetMyTeamID())

--[[
--  gl.LineWidth(2)
  gl.Lighting(false)
  gl.DepthMask(false)
  gl.Color(1,1,1,1)
  gl.PolygonOffset(-4, -4)
  gl.PolygonMode(GL_FRONT_AND_BACK, GL_LINE)
  gl.UnitDef(unitDefID, Spring.GetMyTeamID())
  gl.PolygonMode(GL_FRONT_AND_BACK, GL_FILL)
  gl.PolygonOffset(false)
]]


  gl.Scissor(false)
  gl.PopMatrix()

	RevertModelDrawing()

  -- draw the count text
  gl.Text(count, (xmin + xmax) * 0.5, ymax + 2, fontSize, "oc")

  -- draw the border  (note the half pixel shift for drawing lines)
  gl.Color(1, 1, 1)
  gl.Shape(GL_LINE_LOOP, {
    { v = { xmin + 0.5, ymin + 0.5 }, t = { 0, 1 } },
    { v = { xmax + 0.5, ymin + 0.5 }, t = { 1, 1 } },
    { v = { xmax + 0.5, ymax + 0.5 }, t = { 1, 0 } },
    { v = { xmin + 0.5, ymax + 0.5 }, t = { 0, 0 } },
  })
end


function DrawIconQuad(iconPos, color)
  xmin = rectMinX + (iconSizeX * iconPos)
  xmax = xmin + iconSizeX
  ymin = rectMinY
  ymax = rectMaxY
  gl.Color(color)
  gl.Blending(GL_SRC_ALPHA, GL_ONE)
  gl.Shape(GL_QUADS, {
    { v = { xmin, ymin } },
    { v = { xmax, ymin } },
    { v = { xmax, ymax } },
    { v = { xmin, ymax } },
  })
  gl.Blending(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
end


-------------------------------------------------------------------------------
-------------------------------------------------------------------------------

function widget:MousePress(x, y, button)
  mouseIcon = MouseOverIcon(x, y)
  activePress = (mouseIcon >= 0)
  return activePress
end


-------------------------------------------------------------------------------

local function LeftMouseButton(unitTable)
  local alt, ctrl, meta, shift = Spring.GetModKeyState()
  if (not ctrl) then
    -- select units of icon type
    unitString = ""
    for uid,tmp in pairs(unitTable) do
      unitString = unitString .. " +" .. uid
      if ((alt or meta) and (unitString ~= "")) then
        break  --  only select 1 unit if ALT or META are active
      end
    end
    Spring.SendCommands({"selectunits clear" .. unitString})
  else
    -- select all units of the icon type
    units = Spring.GetTeamUnitsSorted(Spring.GetMyTeamID())
    unitTable = units[unitDefID]
    if (unitTable ~= nil) then
      if (shift) then
        unitString = " "
      else
        unitString = " clear"
      end
      for uid,tmp in pairs(unitTable) do
        unitString = unitString .. " +" .. uid
      end
      Spring.SendCommands({"selectunits " .. unitString})
    end
  end
end


local function MiddleMouseButton(unitTable)
  local alt, ctrl, meta, shift = Spring.GetModKeyState()
  -- center the view
  if (ctrl) then
    -- center the view on the entire selection
    Spring.SendCommands({"viewselection"})
  else
    -- center the view on this type on unit
    local rawunits = Spring.GetSelectedUnits()
    unitString = ""
    for uid,_ in pairs(unitTable) do
      unitString = unitString .. " +" .. uid
    end
    Spring.SendCommands({"selectunits clear" .. unitString})
    Spring.SendCommands({"viewselection"})
    unitString = ""
    for _,uid in ipairs(rawunits) do
      unitString = unitString .. " +" .. uid
    end
    Spring.SendCommands({"selectunits clear" .. unitString})
  end
end


local function RightMouseButton(unitTable)
  local alt, ctrl, meta, shift = Spring.GetModKeyState()
  -- remove selected units of icon type
  unitString = ""
  for uid,tmp in pairs(unitTable) do
    unitString = unitString .. " -" .. uid
    if (ctrl and (unitString ~= "")) then
      break  --  only remove 1 unit if CTRL is active
    end
  end
  Spring.SendCommands({"selectunits" .. unitString})
end


-------------------------------------------------------------------------------

function widget:MouseRelease(x, y, button)
  if (not activePress) then
    return -1
  end
  activePress = false
  icon = MouseOverIcon(x, y)

  units = Spring.GetSelectedUnitsSorted()
  if (units.n ~= unitTypes) then
    return -1  -- discard this click
  end
  units.n = nil

  unitDefID = -1
  unitTable = nil
  index = 0
  for udid,uTable in pairs(units) do
    if (index == icon) then
      unitDefID = udid
      unitTable = uTable
      unitTable.n = 0
      break
    end
    index = index + 1
  end
  if (unitTable == nil) then
    return -1
  end
  
  local alt, ctrl, meta, shift = Spring.GetModKeyState()
  
  if (button == 1) then
    LeftMouseButton(unitTable)
  elseif (button == 2) then
    MiddleMouseButton(unitTable)
  elseif (button == 3) then
    RightMouseButton(unitTable)
  end

  return -1
end


function MouseOverIcon(x, y)
  if (unitTypes <= 0) then return -1 end
  if (x < rectMinX)   then return -1 end
  if (x > rectMaxX)   then return -1 end
  if (y < rectMinY)   then return -1 end
  if (y > rectMaxY)   then return -1 end

  icon = math.floor((x - rectMinX) / iconSizeX)
  -- clamp the icon range
  if (icon < 0) then
    icon = 0
  end
  if (icon >= unitTypes) then
    icon = (unitTypes - 1)
  end
  return icon
end


-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
