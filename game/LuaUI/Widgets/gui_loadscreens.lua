--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    gui_loadscreens.lua
--  brief:   view the mod loadscreens
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name    = "LoadScreens",
    desc    = "View the mod's loadscreens",
    author  = "trepan",
    date    = "May 28, 2007",
    license = "GNU GPL, v2 or later",
    layer   = 0,
    enabled = false  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

include('colors.h.lua')
include("keysym.h.lua")

local minScale = 0.2
local images = {}
local imageNum = 0
local maximized = false

local imagePrefix = ''  --  ':g:'


--------------------------------------------------------------------------------

local vsx, vsy
local xMin, xMax, yMin, yMax

function widget:ViewResize(viewSizeX, viewSizeY)
  vsx = viewSizeX
  vsy = viewSizeY
  xMin = vsx * (0.5 - minScale)
  xMax = vsx * (0.5 + minScale)
  yMin = vsy * (0.5 - minScale)
  yMax = vsy * (0.5 + minScale)
end

widget:ViewResize(widgetHandler:GetViewSizes())


--------------------------------------------------------------------------------

function widget:Initialize()
  images = VFS.DirList('bitmaps/loadpictures/', '*')
  if (#images <= 0) then
    Spring.Echo('no loadscreens to view')
    widgetHandler:RemoveWidget()
    return
  end
  table.sort(images)
  imageNum = 1
end


function widget:Shutdown()
  if (imageNum >= 1) then
    gl.DeleteTexture(imagePrefix .. images[imageNum])
  end
end


--------------------------------------------------------------------------------

local function NextImage()
  local nextNum = imageNum + 1
  if (nextNum > #images) then
    nextNum = 1
  end
  if (nextNum ~= imageNum) then
    gl.DeleteTexture(imagePrefix .. images[imageNum])
  end
  imageNum = nextNum
end
  

local function PrevImage()
  local nextNum = imageNum - 1
  if (nextNum < 1) then
    nextNum = #images
  end
  if (nextNum ~= imageNum) then
    gl.DeleteTexture(imagePrefix .. images[imageNum])
  end
  imageNum = nextNum
end
  

function widget:IsAbove(x, y)
  if (maximized) then
    return true
  end
  if ((x >= xMin) and (x <= xMax) and
      (y >= yMin) and (y <= yMax)) then
    return true
  end
  return false
end


function widget:MousePress(x, y, button)
  if (maximized or widget:IsAbove(x, y)) then
    if (button == 3) then
      NextImage()
      return true
    elseif (button == 1) then
      PrevImage()
      return true
    else
      local a,c,m,s = Spring.GetModKeyState()
      if (c) then
        widgetHandler:RemoveWidget()
        return true
      end
      maximized = not maximized
    end
    return true
  end
  return false
end


function widget:KeyPress(key, mods, isRepeat)
  if (key == KEYSYMS.ESCAPE) then
    widgetHandler:RemoveWidget()
  end
end


function widget:GetTooltip(x, y)
  local nl_grey = '\n\255\180\180\180'
  return 'LoadScreen: ' .. '\255\224\224\128' .. images[imageNum]
         ..nl_grey..'Left Click:      previous image'
         ..nl_grey..'Middle Click:  maximize (toggle)'
         ..nl_grey..'Right Click:     next image'
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:DrawScreen()
  gl.Blending(false)
  gl.Texture(imagePrefix .. images[imageNum])

  if (maximized) then
    gl.Color(1, 1, 1)
    gl.TexRect(0, 0, vsx, vsy)
  else    
    local b
    gl.Texture(false)

    b = 6
    gl.Color(0, 0, 0)
    gl.TexRect(xMin - b, yMin - b, xMax + b, yMax + b)

    b = 4
    gl.Color(1, 1, 1)
    gl.TexRect(xMin - b, yMin - b, xMax + b, yMax + b)

    b = 2
    gl.Color(0, 0, 0)
    gl.TexRect(xMin - b, yMin - b, xMax + b, yMax + b)

    gl.Texture(true)
    gl.Color(1, 1, 1)
    gl.TexRect(xMin, yMin, xMax, yMax)

    gl.Blending(true)
    local xMid = 0.5 * (xMin + xMax)
    gl.Text(GreenStr .. images[imageNum], xMid , yMax + 25, 20, 'oc')
    local ti = gl.TextureInfo(images[imageNum])
    if (ti) then
      gl.Text(CyanStr .. ti.xsize ..
              YellowStr .. ' x ' ..
              CyanStr .. ti.ysize,
              xMid , yMax + 7, 15, 'oc')
    end
  end

  gl.Texture(false)
  gl.Blending(true)
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
