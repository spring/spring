--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    selector.lua
--  brief:   the widget selector, loads and unloads widgets
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "WidgetSelector",
    desc      = "Widget selection widget",
    author    = "trepan",
    date      = "Jan 8, 2007",
    license   = "GNU GPL, v2 or later",
    drawLayer = 3,
    enabled   = true  --  loaded by default?
  }
end

-------------------------------------------------------------------------------
-------------------------------------------------------------------------------

include("colors.h.lua")
include("opengl.h.lua")
include("keysym.h.lua")


local vsx, vsy = widgetHandler:GetViewSizes()

local widgetsList = {}

local fontSize = 15
local fontSpace = 5
local yStep = fontSize + fontSpace

local maxWidth = 0.01
local borderx = yStep * 0.75
local bordery = yStep * 0.75

local midx = vsx * 0.5
local minx = vsx * 0.4
local maxx = vsx * 0.6
local midy = vsy * 0.5
local miny = vsy * 0.4
local maxy = vsy * 0.6


-------------------------------------------------------------------------------

local function UpdateGeometry()
  midx  = vsx * 0.5
  midy  = vsy * 0.5

  local halfWidth = maxWidth * fontSize * 0.5
  minx = math.floor(midx - halfWidth - borderx)
  maxx = math.floor(midx + halfWidth + borderx)

  local ySize = yStep * table.getn(widgetsList)
  miny = math.floor(midy - (0.5 * ySize) - bordery)
  maxy = math.floor(midy + (0.5 * ySize) + bordery)
end
UpdateGeometry()


local function UpdateList()
  local myCount = table.getn(widgetsList)
  if (widgetHandler.knownCount == (myCount + 1)) then
    return
  end

  local myName = widget:GetInfo().name
  maxWidth = 0
  widgetsList = {}
  for name,data in pairs(widgetHandler.knownWidgets) do
    if (name ~= myName) then
      table.insert(widgetsList, { name, data })
      -- look for the maxWidth
      local width = gl.GetTextWidth(name)
      if (width > maxWidth) then
        maxWidth = width
      end
    end
  end

  local myCount = table.getn(widgetsList)
  if (widgetHandler.knownCount ~= (myCount + 1)) then
    error('knownCount mismatch')
  end

  table.sort(widgetsList, function(nd1, nd2)
    return (nd1[1] < nd2[1]) -- sort by name
  end)

  UpdateGeometry()
end


function widget:ViewResize(viewSizeX, viewSizeY)
  vsx = viewSizeX
  vsy = viewSizeY
  UpdateList()
  UpdateGeometry()
end


-------------------------------------------------------------------------------

function widget:KeyPress(key, mods, isRepeat)
  if ((key == KEYSYMS.ESCAPE) or
      ((key == KEYSYMS.F11) and not isRepeat and
       not (mods.alt or mods.ctrl or mods.meta or mods.shift))) then
    widgetHandler:RemoveWidget(self)
    return true
  end
  return false
end


function widget:DrawScreen()
  UpdateList()

  -- draw the header  
  gl.Text("Widget Selector", midx, maxy + 5, fontSize * 1.25, "oc")

  -- draw the box
  gl.Color(0.3, 0.3, 0.3, 1.0)
  gl.Texture("bitmaps/detailtex.bmp")
  local ts = (2.0 / 512)  --  texture scale 
  gl.Shape(GL_QUADS, {
    { v = { minx, miny }, t = { minx * ts, miny * ts } },
    { v = { maxx, miny }, t = { maxx * ts, miny * ts } },
    { v = { maxx, maxy }, t = { maxx * ts, maxy * ts } },
    { v = { minx, maxy }, t = { minx * ts, maxy * ts } } 
  })
  gl.Texture(false)

  -- draw the widget labels
  local mx,my,lmb,mmb,rmb = Spring.GetMouseState()
  local nd = not widgetHandler.tweakMode and self:AboveLabel(mx, my)
  local pointedY = nil
  local pointedEnabled = false
  local pointedName = (nd and nd[1]) or nil
  local posy = maxy - yStep - bordery
  for _,namedata in ipairs(widgetsList) do
    local name = namedata[1]
    local data = namedata[2]
    local color = ''
    local pointed = (pointedName == name)
    if (pointed) then
      pointedY = posy
      pointedEnabled = data.active
      if (lmb or mmb or rmb) then
        color = WhiteStr
      else
        color = (data.active and '\255\128\255\128') or '\255\255\128\128'
      end
    else
      color = (data.active and '\255\064\224\064') or '\255\224\064\064'
    end
    gl.Text(color..name, midx, posy, fontSize, "c")
    posy = posy - yStep
  end

  -- outline the highlighted label
  if (pointedY) then
    if (lmb or mmb or rmb) then
      if (pointedEnabled) then
        gl.Color(1.0, 0.2, 0.2, 0.2)
      else
        gl.Color(0.2, 1.0, 1.0, 0.2)
      end
    else
      gl.Color(0.2, 0.2, 1.0, 0.2)
    end
    local minyp = pointedY - 0.5
    local maxyp = pointedY + yStep + 0.5
    local xn = minx + 0.5
    local xp = maxx - 0.5
    local yn = pointedY - 0.5
    local yp = yn + yStep + 1
    gl.Blending(false)
    gl.Shape(GL_LINE_LOOP, {
      { v = { xn, yn } }, { v = { xp, yn } },
      { v = { xp, yp } }, { v = { xn, yp } }
    })
    xn = minx
    xp = maxx
    yn = yn + 0.5
    yp = yp - 0.5
    gl.Blending(GL_SRC_ALPHA, GL_ONE)
    gl.Shape(GL_QUADS, {
      { v = { xn, yn } }, { v = { xp, yn } },
      { v = { xp, yp } }, { v = { xn, yp } }
    })
    gl.Blending(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
  end

  -- outline the box
  xn = minx - 0.5
  yn = miny - 0.5
  xp = maxx + 0.5
  yp = maxy + 0.5
  gl.Color(1, 1, 1)
  gl.Shape(GL_LINE_LOOP, {
    { v = { xn, yn } }, { v = { xp, yn } },
    { v = { xp, yp } }, { v = { xn, yp } }
  })
  xn = xn - 1
  yn = yn - 1
  xp = xp + 1
  yp = yp + 1
  gl.Color(0, 0, 0)
  gl.Shape(GL_LINE_LOOP, {
    { v = { xn, yn } }, { v = { xp, yn } },
    { v = { xp, yp } }, { v = { xn, yp } }
  })

end


function widget:MousePress(x, y, button)
  UpdateList()
  
  local namedata = self:AboveLabel(x, y)
  if (not namedata) then
    return false
  end
  
  return true
  
end


function widget:MouseMove(x, y, dx, dy, button)
  return false
end


function widget:MouseRelease(x, y, button)
  UpdateList()
  
  local namedata = self:AboveLabel(x, y)
  if (not namedata) then
    return false
  end
  
  local name = namedata[1]
  local data = namedata[2]
  
  if (button == 1) then
    if (data.active) then
      local w = widgetHandler:FindWidget(name)
      if (not w) then return -1 end
      print('Removed:  '..data.filename)
      widgetHandler:RemoveWidget(w)     -- deactivate
      widgetHandler.orderList[name] = 0 -- disable
      widgetHandler:SaveOrderList()
    else
      print('Loading:  '..data.filename)
      local order = widgetHandler.orderList[name]
      if (not order or (order <= 0)) then
        widgetHandler.orderList[name] = 1
      end
      local w = widgetHandler:LoadWidget(data.filename)
      if (not w) then return -1 end
      widgetHandler:InsertWidget(w)
      widgetHandler:SaveOrderList()
    end
  elseif ((button == 2) or (button == 3)) then
    local w = widgetHandler:FindWidget(name)
    if (not w) then return -1 end
    if (button == 2) then
      widgetHandler:LowerWidget(w)
    else
      widgetHandler:RaiseWidget(w)
    end
    widgetHandler:SaveOrderList()
  end
  return -1
end


function widget:AboveLabel(x, y)
  if ((x < minx) or (y < (miny + bordery)) or
      (x > maxx) or (y > (maxy - bordery))) then
    return nil
  end
  local count = table.getn(widgetsList)
  if (count < 1) then return nil end
  
  local i = math.floor(1 + ((maxy - bordery) - y) / yStep)
  if     (i < 1)     then i = 1
  elseif (i > count) then i = count end
  
  return widgetsList[i]
end


function widget:IsAbove(x, y)
  UpdateList()
  if ((x < minx) or (x > maxx) or
      (y < miny) or (y > maxy)) then
    return false
  end
  return true
end


function widget:GetTooltip(x, y)
  UpdateList()  
  local namedata = self:AboveLabel(x, y)
  if (not namedata) then
    return '\255\200\255\200'..'Widget Selector\n'..
           '\255\255\200\200'..'RMB: raise widget\n'..
           '\255\200\200\255'..'MMB: lower widget'
  end

  local name = namedata[1]
  local data = namedata[2]
  
  local tt = (data.active and GreenStr) or RedStr
  tt = tt..name..'\n'
  tt = data.desc   and tt..WhiteStr..data.desc..'\n'
  tt = data.author and tt..BlueStr..'Author:  '..CyanStr..data.author..'\n'
  tt = tt..MagentaStr..data.basename
  return tt
end


-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
