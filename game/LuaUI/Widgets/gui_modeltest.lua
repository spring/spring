--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    gui_modeltest.lua
--  brief:   test widget for the obj2lua model format/loader
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "ModelTest",
    desc      = "test widget for the obj2lua model format/loader",
    author    = "trepan",
    date      = "Mar 16, 2007",
    license   = "GNU GPL, v2 or later",
    layer     = 5,
    enabled   = false  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Headers and top-level variables
--

local LoadModel = include("loadmodel.lua")


local clip    = (1 > 0)
local merged  = (1 > 0)
local rotate  = (1 > 0)
local revolve = (-1 > 0)
local useDepthTest = (1 > 0)
local useDepthMask = (1 > 0)


local displayLists = {}


local ImagePath = LUAUI_DIRNAME .. "Images/"


local model, scale, fixYZ = "octo.lua",      1,      false
local model, scale, fixYZ = "metro.lua",     1,      false
local model, scale, fixYZ = "bunny10k.lua",  10000,  false
local model, scale, fixYZ = "ship.lua",      64,     false
local model, scale, fixYZ = "loudesk.lua",   1,      true
local model, scale, fixYZ = "overlord.lua",  1,      true
local model, scale, fixYZ = "gear.lua",      100,    false
local model, scale, fixYZ = "cow.lua",       64,     false
local model, scale, fixYZ = "styleTank.lua", 1640,   false
local model, scale, fixYZ = "chillbox.lua",  1,      true
local model, scale, fixYZ = "atlantis.lua",  1,      true
local model, scale, fixYZ = "s4.lua",        100,    false
local model, scale, fixYZ = "s4.luac",       100,    false
local model, scale, fixYZ = "colors.lua",    100,    false


local msx = Game.mapSizeX
local msz = Game.mapSizeZ

local vsx, vsy = widgetHandler:GetViewSizes()
function widget:ViewResize(viewSizeX, viewSizeY)
  vsx = viewSizeX
  vsy = viewSizeY
end

local px, py, pz, radians = 0,0,0,0


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function FreeModel()
  for _,dl in ipairs(displayLists) do
    dl.Free()
  end
end


local function LoadNewModel(name)

  print("LoadNewModel")

  FreeModel()

  local t1 = Spring.GetTimer()
  local objs, mats, verts, txcds, norms, colors, exts
        = LoadModel("Models/" .. name)

  if (merged) then
    objs = { objs.Merge(objs) }
    objs[1].name = "merged"
  end
  local t2 = Spring.GetTimer()
  print("Loading model took:  " .. Spring.DiffTimers(t2, t1))

  print("MODELTEST:  " .. tostring(objs))
  if (objs == nil) then
    widgetHandler:RemoveWidget()
  end

  -- setup the image path
  for matname,mat in pairs(mats) do
    if (mat.texture and (mat.texture ~= "")) then
      mat.texture = ImagePath .. mat.texture
    end
    -- clean out the ambient colors
--    mat.ambient = {0,0,0,0}
--    mat.emission = {0,0,0,0}
  end

  -- make the display lists
  local t1 = Spring.GetTimer()
  for i,obj in ipairs(objs) do
    local listNoMat  = gl.CreateList(obj.Draw, true, useDepthMask)
    local listUseMat = gl.CreateList(obj.Draw, false, useDepthMask)
    print("ListIDs = " .. listUseMat .. " " .. listNoMat)
    local tbl = {}
    tbl.name = obj.name
    tbl.Draw      = function() gl.CallList(listUseMat) end
    tbl.DrawNoMat = function() gl.CallList(listNoMat) end
    tbl.Free = function()
      gl.DeleteList(listNoMat)
      gl.DeleteList(listUseMat)
    end
    displayLists[i] = tbl
  end
  local t2 = Spring.GetTimer()
  print("DisplayListing model took:  " .. Spring.DiffTimers(t2, t1))

  -- set the scale
  local xdiff = (exts.xmax - exts.xmin)
  local ydiff = (exts.ymax - exts.ymin)
  local zdiff = (exts.zmax - exts.zmin)
  local maxdiff = xdiff
  if (ydiff > maxdiff) then maxdiff = ydiff end
  if (zdiff > maxdiff) then maxdiff = zdiff end
  scale = 2048 / maxdiff 

  fixYZ = ((ydiff > xdiff) or (ydiff > zdiff))
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function ModelCmd(cmd, line, words)
  print("ModelCmd")
  LoadNewModel(words[1])
end


function widget:Initialize()
  LoadNewModel(model)
  widgetHandler:AddAction("luamodel", ModelCmd)
end


function widget:Shutdown()
  FreeModel()
  widgetHandler:RemoveAction("luamodel")
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:Update()
  py = 500
  local r = 1000
  local cx = 0.5 * Game.mapSizeX
  local cz = 0.5 * Game.mapSizeZ
  local time = widgetHandler:GetHourTimer()
  radians = (math.pi * 2 * time * 0.1) % (math.pi * 2)
  if (revolve) then
    local cos_val = math.cos(radians)
    local sin_val = math.sin(radians)
    px = cx + (r * sin_val)
    pz = cz + (r * cos_val)
  else
    px = cx
    pz = cz
  end
end


function Draw(useMat, mode)
  if (useDepthTest) then
    gl.DepthTest(true)
  end
  gl.Culling(GL.BACK)
  gl.PushMatrix()
  gl.Translate(px, py, pz)
  gl.PolygonMode(GL.FRONT_AND_BACK, mode)

  if (clip) then
    local hourTime = widgetHandler:GetHourTimer()
    local rate = 4
    local dist = msx * (0.5 - ((hourTime % rate) / rate))
    gl.ClipPlane(1,  1, 0, 0, dist)
    gl.ClipPlane(2, -1, 0, 0, -dist + (msx * 0.1))
  end

  for i,dl in ipairs(displayLists) do
    gl.PushMatrix()

    if (rotate or revolve) then
      gl.Rotate(radians * 180 / math.pi, 0, 1, 0)
    end
    if (revolve) then
      gl.Rotate(-30, 1, 0, 0)
    end
    if (fixYZ) then
      gl.Rotate(-90, 1, 0, 0)
    end
    gl.Scale(scale, scale, scale)

    if (useMat) then
      dl.Draw()
    else
      dl.DrawNoMat()
    end

    gl.PopMatrix()
  end

  gl.PolygonMode(GL.FRONT_AND_BACK, mode)
  gl.PopMatrix()

  if (clip) then
    gl.ClipPlane(1, false)
    gl.ClipPlane(2, false)
  end

  gl.ResetState()
end


function widget:DrawWorld()
  gl.Color(1,1,1)
  gl.ResetState()
  Draw(true, GL.FILL)
end
function widget:DrawWorldReflection() Draw(true, GL.FILL) end
function widget:DrawWorldRefraction() Draw(true, GL.FILL) end
function widget:DrawWorldShadow()
  gl.DepthMask(true)
  Draw(false, GL.FILL)
end
function widget:DrawInMiniMap()
  -- this will probably be a common display
  -- list for widgets that use DrawInMiniMap()
  gl.PushMatrix()
  gl.LoadIdentity()
  gl.Translate(0, 1, 0)
  gl.Scale(1 / msx, 1 / msz, 1)
  gl.Rotate(90, 1, 0, 0)

  gl.Color(0.6, 0.8, 0.6, 1)
  Draw(false, GL.FILL)

  gl.PopMatrix()
end
              

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

                  