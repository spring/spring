--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    modui_dialog.lua
--  brief:   controls how mods' LuaUIs are used
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

Spring.SendCommands({
  "resbar 0",
  "console 0",
  "tooltip 0",
  "minimap slavemode 1",
  "minimap minimize 1",
})
local function RevertHiding()
  Spring.SendCommands({
    "resbar 1",
    "console 1",
    "tooltip 1",
    "minimap slavemode 0",
    "minimap minimize 0",
  })
end


local LT = {}  --  local table
setmetatable(LT, { __index = _G})

local function IncludeFile(filename, t)
  local chunk, err = loadfile(LUAUI_DIRNAME .. filename)
  if (chunk == nil) then
    Spring.SendCommands({
      'echo Failed to load: ' .. filename .. '(' .. err .. ')',
      'luaui disable'
    })
    LayoutButtons = function () return 'disabled' end
  else
    if (t ~= nil) then
	    setfenv(chunk, t)
	  end
    chunk()
  end
end

IncludeFile('Headers/colors.h.lua', LT);
IncludeFile('Headers/opengl.h.lua', LT);
IncludeFile('Headers/keysym.h.lua', LT);
IncludeFile('savetable.lua', LT);


local Echo = function(msg) Spring.SendCommands({"echo " .. msg}) end


--------------------------------------------------------------------------------
--
--  Local variables  (with namespace encapsulation)
--

local gl = Spring.Draw

local vsx = -1
local vsy = -1

local frame = {}

local buttons = {
  No     = { tooltip = "Do not use this Mod's LuaUI for this game" },
  Yes    = { tooltip = "Use this Mod's LuaUI for this game" },
  Never  = { tooltip = "Never use this Mod's LuaUI" },
  Always = { tooltip = "Always use this Mod's LuaUI" }
}
for name,b in pairs(buttons) do
  b.textWidth = gl.GetTextWidth(name)
end

local modStr = '\n' .. LT.YellowStr .. LT.Game.modName
for _,b in pairs(buttons) do
--???  b.tooltip = b.tooltip .. modStr
end
local tooltip = "Use Mod's LuaUI?" .. modStr


--------------------------------------------------------------------------------

local function MouseOverButton(x, y, button)
  if (x < button.minX) then return false end
  if (x > button.maxX) then return false end
  if (y < button.minY) then return false end
  if (y > button.maxY) then return false end
  return true
end


--------------------------------------------------------------------------------

local function SetupWidgetGeometry()
  frame.minX = (vsx * 0.5) - 128
  frame.maxX = (vsx * 0.5) + 128
  frame.minY = (vsy * 0.25) - 64
  frame.maxY = (vsy * 0.25) + 64
  local fSizeX = frame.maxX - frame.minX
  local fSizeY = frame.maxY - frame.minY
  frame.midX = (frame.minX + frame.maxX) * 0.5
  frame.midY = (frame.minY + frame.maxY) * 0.5

  local gap = (fSizeX * 0.05)
  local xa = frame.minX + gap
  local xb = frame.midX - (gap * 0.5)
  local xc = frame.midX + (gap * 0.5)
  local xd = frame.maxX - gap
  local ya = frame.minY + gap
  local yb = frame.midY - (gap * 0.5)
  local yc = frame.midY + (gap * 0.5)
  local yd = frame.maxY - gap
  
  local b
  b = buttons.No;     b.minX = xc; b.maxX = xd; b.minY = yc; b.maxY = yd
  b = buttons.Yes;    b.minX = xa; b.maxX = xb; b.minY = yc; b.maxY = yd
  b = buttons.Never;  b.minX = xc; b.maxX = xd; b.minY = ya; b.maxY = yb
  b = buttons.Always; b.minX = xa; b.maxX = xb; b.minY = ya; b.maxY = yb

  for _,b in pairs(buttons) do
    b.midX = (b.maxX + b.minX) * 0.5
    b.textY = b.minY + 8
    b.textSize = 0.5 * ((fSizeY * 0.5) - (gap * 1.5))
  end
end


local function DrawButton(b)
  gl.Shape(LT.GL_QUADS, {
    { v = { b.minX, b.minY } },
    { v = { b.maxX, b.minY } },
    { v = { b.maxX, b.maxY } },
    { v = { b.minX, b.maxY } }
  })
end


local function DrawBorder(b)
  gl.Shape(LT.GL_LINE_LOOP, {
    { v = { b.minX - 0.5, b.minY - 0.5 } },
    { v = { b.maxX + 0.5, b.minY - 0.5 } },
    { v = { b.maxX + 0.5, b.maxY + 0.5 } },
    { v = { b.minX - 0.5, b.maxY + 0.5} }
  })
end


--------------------------------------------------------------------------------
--
--
--

local function ClearLocalTables()
  DrawScreenItems = nil
  KeyPress = nil
  KeyRelease = nil
  MousePress = nil
  MouseRelease = nil
  IsAbove = nil
  GetTooltip = nil
end


local function LoadUserLuaUI()
  RevertHiding()
  ClearLocalTables()

  USER_FILENAME   = LUAUI_DIRNAME .. 'main.lua'

  local file, err = io.open(USER_FILENAME, 'r')
  if (file == nil) then
    LayoutButtons = function () return 'disabled' end
    return
  else
    file:close()
  end

  local chunk, err = loadfile(USER_FILENAME)
  if (chunk == nil) then
    Spring.SendCommands({
      'echo Failed to load: ' .. USER_FILENAME .. ' (' .. err .. ')',
      'luaui disable'
    })
    LayoutButtons = function () return 'disabled' end
  else
    chunk()
  end
end


local function LoadModLuaUI()
  RevertHiding()
  ClearLocalTables()
    
  local MOD_FILENAME = 'ModUI/main.lua'

  local modText = Spring.LoadTextVFS(MOD_FILENAME)
  if (modText == nil) then
    Echo('Failed to load ' .. MOD_FILENAME)
    LoadUserLuaUI()
    return
  end

  local chunk, err = loadstring(modText)
  if (chunk == nil) then
    Echo('Failed to load ' .. MOD_FILENAME .. ': (' .. err .. ')')
    LoadUserLuaUI()
    return
  else
    LUAUI_DIRNAME = nil
    local success, err = pcall(chunk)
    if (not success) then
      Echo('Failed to load ' .. MOD_FILENAME .. ': (' .. err .. ')')
      LoadUserLuaUI()
    end
  end
end


local function SaveModPerm(state)
  local perms
  local filename = LUAUI_DIRNAME..'Config/modui_list.lua'
  local chunk, err = loadfile(filename)
  if (chunk ~= nil) then
    local tmp = {}
    setfenv(chunk, tmp)
    perms = chunk()
  end
  if (perms == nil) then
    perms = {}
  end
  perms[Game.modName] = state
  LT.table.save(perms, filename, '-- Mod LuaUI Permissions')
end


--------------------------------------------------------------------------------

local timer = 0

function DrawScreenItems(viewSizeX, viewSizeY)
  if ((vsx ~= viewSizeX) or (vsy ~= viewSizeY)) then
    vsx = viewSizeX
    vsy = viewSizeY
    SetupWidgetGeometry()
  end
  
  local overButton = nil
  local x,y,lmb = Spring.GetMouseState()
  for _,b in pairs(buttons) do
    if (MouseOverButton(x, y, b)) then
      overButton = b
      break
    end
  end
  
  gl.Color(0.1, 0.1, 0.1, 0.7)
--  gl.Color(0, 0, 0)
  gl.Shape(LT.GL_QUADS, {
    { v = {   0,   0 } }, { v = { vsx,   0 } },
    { v = { vsx, vsy } }, { v = {   0, vsy } }
  })
  
  -- draw frame
  gl.Color(0.35, 0.35, 0.35)
  DrawButton(frame)

  -- draw frame border
  timer = math.mod(timer + Spring.GetLastFrameSeconds(), 60.0)
  local blink = math.abs(0.5 - math.mod(timer * 1.5, 1.0)) * 2.0
  gl.LineWidth(3.0)
  gl.Color(0.3 + (blink * 0.7), 0, 0)
  DrawBorder(frame)
  gl.LineWidth(1.0)

  -- draw header  
  gl.Text(LT.YellowStr.."Use Mod's LuaUI?", frame.midX, frame.maxY + 4, 24, "oc")

  -- draw trailer
  local tt = (overButton and overButton.tooltip) or "Pick one"
  gl.Text(tt, frame.midX, frame.minY - 28, 18, "oc")

  -- draw buttons
  gl.Color(0.3, 0.3, 0.5, 0.9)
  for _,b in pairs(buttons) do
    DrawButton(b)
  end

  -- draw borders
  gl.Color(1, 1, 1)
  for _,b in pairs(buttons) do
    DrawBorder(b)
  end

  -- draw labels
  gl.Color(0, 1, 0)
  for name,b in pairs(buttons) do
    gl.Text(LT.GreenStr..name, b.midX, b.textY, b.textSize, "c")
  end
  
  -- draw highlight
  if (overButton) then
    gl.Blending(LT.GL_SRC_ALPHA, LT.GL_ONE)
    if (lmb) then
      gl.Color(1, 0, 0, 0.5)
    else 
      gl.Color(0, 0, 1, 0.5)
    end
    DrawButton(overButton)
    gl.Blending(LT.GL_SRC_ALPHA, LT.GL_ONE_MINUS_SRC_ALPHA)
  end
end


function KeyPress(key)
  local ks = LT.KEYSYMS
  if (key == ks.ESCAPE) then   -- No
    Echo("Not using Mod's LuaUI")
    LoadUserLuaUI()
    return true
  end
  return false  --  ??? true?
end


function KeyRelease()
  return false  --  ??? true?
end


function MousePress(x, y, button)
  return true  --  eat them
end


function MouseRelease(x, y, button)
  if (not next(frame)) then
    return -1
  end
  if (button == 1) then
    if (MouseOverButton(x, y, buttons.No)) then
      SaveModPerm(nil)
      LoadUserLuaUI()
    elseif (MouseOverButton(x, y, buttons.Yes)) then
      SaveModPerm(nil)
      LoadModLuaUI()
    elseif (MouseOverButton(x, y, buttons.Never)) then
      SaveModPerm(false)
      LoadUserLuaUI()
    elseif (MouseOverButton(x, y, buttons.Always)) then
      SaveModPerm(true)
      LoadModLuaUI()
    end
  end
  return -1  --  not a command click
end


function IsAbove(x, y)
  if (not next(frame)) then
    return false
  end
  for _,b in pairs(buttons) do
    if (MouseOverButton(x, y, b)) then
      return true
    end
  end
  return true
end


function GetTooltip(x, y)
  if (not next(frame)) then
    return false
  end
  for _,b in pairs(buttons) do
    if (MouseOverButton(x, y, b)) then
      return b.tooltip
    end
  end
  return tooltip
end


--------------------------------------------------------------------------------
