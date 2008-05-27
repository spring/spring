--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    gui_game_setup.lua
--  brief:   alternate game setup rendering
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "GameSetup",
    desc      = "Alternate game setup rendering",
    author    = "trepan",
    date      = "Aug 25, 2007",
    license   = "GNU GPL, v2 or later",
    layer     = -9,
    enabled   = false  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

include("colors.h.lua")
include("fonts.lua")

local flags = include("flags.lua")
local flagsSpecs = flags.specs
local flagsTexture = ':n:'..LUAUI_DIRNAME..'Images/'..flags.texture


--------------------------------------------------------------------------------

local gameState = ''
local ready = true
local playerStates = {}

local readyStr = 'Ready'
local readySize = 18
local readyBorder = 8

local myReady = false

local mouseActive = false

local vsx, vsy = widgetHandler:GetViewSizes()

local font = 'FreeMonoBold'
local fontSize = 16
local fontGap  = 3
local fontName = ':n:'..LUAUI_DIRNAME..'Fonts/'..font..'_'..fontSize

local stateColorStrs = {
  missing  = string.char(255, 255,   1,   1),
  notready = string.char(255, 255, 255,   1),
  ready    = string.char(255,   1, 255,   1),
}


--------------------------------------------------------------------------------


local x1, y1, x2, y2

function widget:ViewResize(viewSizeX, viewSizeY)
  vsx = viewSizeX
  vsy = viewSizeY
  local w = readySize * gl.GetTextWidth(readyStr)
  x1 = ((vsx - w) * 0.5) - readyBorder
  x2 = ((vsx + w) * 0.5) + readyBorder
  y2 = vsy * 0.65
  y1 = y2 - readySize - (2 * readyBorder)
end

widget:ViewResize(vsx, vsy)

--------------------------------------------------------------------------------

function widget:GameSetup(_gameState, _ready, _playerStates)
  gameState    = _gameState
  ready        = _ready
  playerStates = _playerStates
  local tmp = myReady
  myReady = false
  return true, tmp
end


local function InBox(x, y)
  if ((x > x1) and (y > y1) and (x < x2) and (y < y2)) then
    return true
  end
  return false
end


local function DrawPlayerList()
  if (next(playerStates) == nil) then
    return
  end
  local sorted = {}
  for pid, pState in pairs(playerStates) do
    local name, _, _, _, _, _, _, country, rank = Spring.GetPlayerInfo(pid)
    table.insert(sorted, { name, pState, country, rank })
  end
  table.sort(sorted, function(a, b)
    if (a[2] == b[2]) then
      return (a[1] < b[1])
    end
    return (a[2] < b[2])
  end)
  
  local x = math.floor(fontSize * gl.GetTextWidth(' '))
  local y = math.floor(vsy * 0.6);
  local xt = x + (flags.xsize)
  gl.Text('Players:', x, y + 2, fontSize, 'on')

  for _, info in ipairs(sorted) do
    local name    = info[1]
    local state   = info[2]
    local country = info[3] --and 'ec'
    local rank    = info[3]
    y = y - (fontSize + fontGap)

    local flag = flagsSpecs[country] or flagsSpecs['xx']

    local yf = y-- + (0.5 * (fontSize - flags.ysize))
    if (flag and gl.Texture(flagsTexture)) then
      gl.Color(1, 1, 1)
      gl.TexRect(x, yf, x + flags.xsize, yf + flags.ysize,
                 flag[1], flag[2], flag[3], flag[4])
      gl.Texture(false)
    end

    local colorStr = stateColorStrs[state] or BlueStr
    local str = colorStr .. ' '
    str = str .. name
    --str = str .. WhiteStr .. '  (' .. string.lower(flag.name) .. ')'
    if (fontHandler.UseFont(fontName)) then
      fontHandler.Draw(str, xt, y)
    else 
      gl.Text(str, xt, y, fontSize, 'o')
    end
  end
end


local function DrawReadyButton()
  local t = widgetHandler:GetHourTimer()
  local a = math.abs(0.5 - (t * 4) % 1)
  local color
  local readyColorStr = ''
  if (InBox(Spring.GetMouseState())) then
    if (mouseActive) then
      color = { 0.0, 1.0, 0.0, a }
      readyColorStr = GreenStr
    else
      color = { 0.8, 0.8, 0.0, a }
    end
  else
    if (mouseActive) then
      color = { 0.0, 0.0, 1.0, a }
    else
      color = { 1.0, 0.0, 0.0, a }
    end
  end
  gl.Color(color)
  gl.Rect(x1, y1, x2, y2)

  gl.Color(color[1], color[2], color[3], 0.5)
  gl.LineWidth(2)
  gl.PolygonMode(GL.FRONT_AND_BACK, GL.LINE)
  gl.Blending(GL.SRC_ALPHA, GL.ONE)
  gl.Rect(x1, y1, x2, y2)
  gl.Blending(GL.SRC_ALPHA, GL.ONE_MINUS_SRC_ALPHA)
  gl.PolygonMode(GL.FRONT_AND_BACK, GL.FILL)
  gl.LineWidth(1)
  gl.Text(readyColorStr .. readyStr, x1 + readyBorder, y1 + readyBorder - 2, readySize, 'o')
end


function widget:DrawScreen()
  if (Spring.GetGameSeconds() > 0) then
    widgetHandler:RemoveWidget()
    return
  end

  gl.Text(gameState, vsx * 0.5, (vsy * 0.66), fontSize * 1.5, 'oc')

  DrawPlayerList()

  if (not ready) then
    DrawReadyButton()
  end
end


local function InBox(x, y)
  if ((x > x1) and (y > y1) and (x < x2) and (y < y2)) then
    return true
  end
  return false
end


function widget:MousePress(x, y, button)
  if (button ~= 1) then
    return false
  end
  if (InBox(x, y)) then
    mouseActive = true
    return true
  end
  return false
end


function widget:MouseRelease(x, y, button)
  mouseActive = false
  if (InBox(x, y)) then
    myReady = true
  end
  return -1
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
