--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "chiliGUI",
    desc      = "hot GUI Framework",
    author    = "jK & quantum",
    date      = "WIP",
    license   = "GPLv2",
    layer     = 1000,
    enabled   = true  --  loaded by default?
  }
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local Chili = VFS.Include(LUAUI_DIRNAME.."Widgets/chiligui/chiligui.lua")

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local screen0 = Chili.Screen:New{}
local th --//= Chili.TextureHandler
local tk = Chili.TaskHandler

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

--// Export Widget Globals
WG.Chili = Chili
WG.Chili.Screen0 = screen0

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:DrawScreen()
  if (not th) then
    th = Chili.TextureHandler
    th.Initialize()
  end
  th.Update()

  gl.PushMatrix()
  local vsx,vsy = gl.GetViewSizes()
  gl.Translate(0,vsy,0)
  gl.Scale(1,-1,1)
  screen0:Draw()
  gl.PopMatrix()
end


function widget:Update()
  --th.Update()
  tk.Update()
end


function widget:IsAbove(x,y)
  if screen0:IsAbove(x,y) then
    return true
  end
end


local mods = {}
function widget:MousePress(x,y,button)
  local alt, ctrl, meta, shift = Spring.GetModKeyState()
  mods.alt=alt; mods.ctrl=ctrl; mods.meta=meta; mods.shift=shift;

  if screen0:MouseDown(x,y,button,mods) then
    return true
  end
end


function widget:MouseRelease(x,y,button)
  local alt, ctrl, meta, shift = Spring.GetModKeyState()
  mods.alt=alt; mods.ctrl=ctrl; mods.meta=meta; mods.shift=shift;

  if screen0:MouseUp(x,y,button,mods) then
    return true
  end
end


function widget:MouseMove(x,y,dx,dy,button)
  local alt, ctrl, meta, shift = Spring.GetModKeyState()
  mods.alt=alt; mods.ctrl=ctrl; mods.meta=meta; mods.shift=shift;

  if screen0:MouseMove(x,y,dx,dy,button,mods) then
    return true
  end
end


function widget:MouseWheel(up,value)
  local x,y = Spring.GetMouseState()
  local alt, ctrl, meta, shift = Spring.GetModKeyState()
  mods.alt=alt; mods.ctrl=ctrl; mods.meta=meta; mods.shift=shift;
  

  if screen0:MouseWheel(x,y,up,value,mods) then
    return true
  end
end
