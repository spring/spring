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
    layer     = 0,
    enabled   = false  --  loaded by default?
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
local currentDef = nil

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
    currentDef  = nil
    return
  end
  
  SetupDimensions(unitTypes)

  -- unit model rendering uses the depth-buffer
  gl.Clear(GL.DEPTH_BUFFER_BIT)

  local x,y,lb,mb,rb = Spring.GetMouseState()
  local mouseIcon = MouseOverIcon(x, y)

  -- draw the buildpics
  unitCounts.n = nil  
  local icon = 0
  for udid,count in pairs(unitCounts) do
    DrawUnitDefIcon(udid, icon, count)
    if (icon == mouseIcon) then
      currentDef = UnitDefs[udid]
    end
    icon = icon + 1
  end

  -- draw the highlights
  if (not widgetHandler:InTweakMode() and (mouseIcon >= 0)) then
    if (lb or mb or rb) then
      DrawIconQuad(mouseIcon, { 1, 0, 0, 0.333 })  --  red highlight
    else
      DrawIconQuad(mouseIcon, { 0, 0, 1, 0.333 })  --  blue highlight
    end
  end
end


function SetupDimensions(count)
  local xmid = vsx * 0.5
  local width = math.floor(iconSizeX * count)
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
  if (ud.name == 'corcomXXX') then -- FIXME
    print(xSize, ySize, zSize)
    print(d.minx, d.maxx)
    print(d.miny, d.maxy)
    print(d.minz, d.maxz)
  end

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
  gl.Shape(GL.QUADS, {
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
  local timer = 1.5 * widgetHandler:GetHourTimer()
  gl.Rotate(math.cos(0.5 * math.pi * timer) * 60.0, 0, 1, 0)

  CenterUnitDef(unitDefID)
  
  local scribe = false
  if (scribe) then
    gl.Lighting(false)
    gl.Color(0,0,0,1)
  end

  gl.UnitShape(unitDefID, Spring.GetMyTeamID())

  if (scribe) then
--    gl.LineWidth(0.1)
    gl.Lighting(false)
    gl.DepthMask(false)
    gl.Color(1,1,1,1)
    gl.PolygonOffset(-4, -4)
    gl.PolygonMode(GL.FRONT_AND_BACK, GL.LINE)
    gl.UnitDef(unitDefID, Spring.GetMyTeamID())
    gl.PolygonMode(GL.FRONT_AND_BACK, GL.FILL)
    gl.PolygonOffset(false)
--    gl.LineWidth(1.0)
  end

  gl.Scissor(false)
  gl.PopMatrix()

	RevertModelDrawing()

  -- draw the count text
  gl.Text(count, (xmin + xmax) * 0.5, ymax + 2, fontSize, "oc")

  -- draw the border  (note the half pixel shift for drawing lines)
  gl.Color(1, 1, 1)
  gl.Shape(GL.LINE_LOOP, {
    { v = { xmin + 0.5, ymin + 0.5 }, t = { 0, 1 } },
    { v = { xmax + 0.5, ymin + 0.5 }, t = { 1, 1 } },
    { v = { xmax + 0.5, ymax + 0.5 }, t = { 1, 0 } },
    { v = { xmin + 0.5, ymax + 0.5 }, t = { 0, 0 } },
  })
end


function DrawIconQuad(iconPos, color)
  local xmin = rectMinX + (iconSizeX * iconPos)
  local xmax = xmin + iconSizeX
  local ymin = rectMinY
  local ymax = rectMaxY
  gl.Color(color)
  gl.Blending(GL.SRC_ALPHA, GL.ONE)
  gl.Shape(GL.QUADS, {
    { v = { xmin, ymin } },
    { v = { xmax, ymin } },
    { v = { xmax, ymax } },
    { v = { xmin, ymax } },
  })
  gl.Blending(GL.SRC_ALPHA, GL.ONE_MINUS_SRC_ALPHA)
end


-------------------------------------------------------------------------------
-------------------------------------------------------------------------------

function widget:MousePress(x, y, button)
  mouseIcon = MouseOverIcon(x, y)
  activePress = (mouseIcon >= 0)
  return activePress
end


-------------------------------------------------------------------------------

local function LeftMouseButton(unitDefID, unitTable)
  local alt, ctrl, meta, shift = Spring.GetModKeyState()
  if (not ctrl) then
    -- select units of icon type
    if (alt or meta) then
      Spring.SelectUnitArray({ unitTable[1] })  -- only 1
    else
      Spring.SelectUnitArray(unitTable)
    end
  else
    -- select all units of the icon type
    local sorted = Spring.GetTeamUnitsSorted(Spring.GetMyTeamID())
    local units = sorted[unitDefID]
    if (units) then
      Spring.SelectUnitArray(units, shift)
    end
  end
end


local function MiddleMouseButton(unitDefID, unitTable)
  local alt, ctrl, meta, shift = Spring.GetModKeyState()
  -- center the view
  if (ctrl) then
    -- center the view on the entire selection
    Spring.SendCommands({"viewselection"})
  else
    -- center the view on this type on unit
    local selUnits = Spring.GetSelectedUnits()
    Spring.SelectUnitArray(unitTable)
    Spring.SendCommands({"viewselection"})
    Spring.SelectUnitArray(selUnits)
  end
end


local function RightMouseButton(unitDefID, unitTable)
  local alt, ctrl, meta, shift = Spring.GetModKeyState()
  -- remove selected units of icon type
  local selUnits = Spring.GetSelectedUnits()
  local map = {}
  for _,uid in ipairs(selUnits) do map[uid] = true end
  for _,uid in ipairs(unitTable) do
    map[uid] = nil
    if (ctrl) then break end -- only remove 1 unit
  end
  Spring.SelectUnitMap(map)
end


-------------------------------------------------------------------------------

function widget:MouseRelease(x, y, button)
  if (not activePress) then
    return -1
  end
  activePress = false
  local icon = MouseOverIcon(x, y)

  local units = Spring.GetSelectedUnitsSorted()
  if (units.n ~= unitTypes) then
    return -1  -- discard this click
  end
  units.n = nil

  local unitDefID = -1
  local unitTable = nil
  local index = 0
  for udid,uTable in pairs(units) do
    if (index == icon) then
      unitDefID = udid
      unitTable = uTable
      break
    end
    index = index + 1
  end
  if (unitTable == nil) then
    return -1
  end
  
  local alt, ctrl, meta, shift = Spring.GetModKeyState()
  
  if (button == 1) then
    LeftMouseButton(unitDefID, unitTable)
  elseif (button == 2) then
    MiddleMouseButton(unitDefID, unitTable)
  elseif (button == 3) then
    RightMouseButton(unitDefID, unitTable)
  end

  return -1
end


function MouseOverIcon(x, y)
  if (unitTypes <= 0) then return -1 end
  if (x < rectMinX)   then return -1 end
  if (x > rectMaxX)   then return -1 end
  if (y < rectMinY)   then return -1 end
  if (y > rectMaxY)   then return -1 end

  local icon = math.floor((x - rectMinX) / iconSizeX)
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

function widget:IsAbove(x, y)
  local icon = MouseOverIcon(x, y)
  if (icon < 0) then
    return false
  end
  return true
end


function widget:GetTooltip(x, y)
  local ud = currentDef
  if (not ud) then
    return ''
  end
  return ud.humanName .. ' ' .. ud.tooltip
end


-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
