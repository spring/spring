--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "Chili Framework",
    desc      = "Hot GUI Framework (DO NOT DISABLE)",
    author    = "jK & quantum",
    date      = "WIP",
    license   = "GPLv2",
    layer     = 1000,
    enabled   = true,  --  loaded by default?
    handler   = true,
    api       = true,
	alwaysStart = true,
  }
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local Chili
local screen0
local th
local tk

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:Initialize()
  Chili = VFS.Include(LUAUI_DIRNAME.."Widgets/chili/core.lua")

  screen0 = Chili.Screen:New{}
  th = Chili.TextureHandler
  tk = Chili.TaskHandler

  --// Export Widget Globals
  WG.Chili = Chili
  WG.Chili.Screen0 = screen0

  --// do this after the export to the WG table!
  --// because other widgets use it with `parent=Chili.Screen0`,
  --// but chili itself doesn't handle wrapped tables correctly (yet)
  screen0 = Chili.DebugHandler.SafeWrap(screen0)
end

function widget:Shutdown()
  --table.clear(Chili) the Chili table also is the global of the widget so it contains a lot more than chili's controls (pairs,select,...)
  WG.Chili = nil
end

function widget:Dispose()
  screen0:Dispose()
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:DrawScreen()
  if (not screen0:IsEmpty()) then
    gl.PushMatrix()
    local vsx,vsy = gl.GetViewSizes()
    gl.Translate(0,vsy,0)
    gl.Scale(1,-1,1)
    screen0:Draw()
    gl.PopMatrix()
  end
end


function widget:TweakDrawScreen()
  if (not screen0:IsEmpty()) then
    gl.PushMatrix()
    local vsx,vsy = gl.GetViewSizes()
    gl.Translate(0,vsy,0)
    gl.Scale(1,-1,1)
    screen0:TweakDraw()
    gl.PopMatrix()
  end
end


function widget:Update()
  tk.Update()
end


function widget:DrawGenesis()
  th.Update()
end


function widget:IsAbove(x,y)
  return (not screen0:IsEmpty()) and screen0:IsAbove(x,y)
end


local mods = {}
function widget:MousePress(x,y,button)
  local alt, ctrl, meta, shift = Spring.GetModKeyState()
  mods.alt=alt; mods.ctrl=ctrl; mods.meta=meta; mods.shift=shift;

  return screen0:MouseDown(x,y,button,mods)
end


function widget:MouseRelease(x,y,button)
  local alt, ctrl, meta, shift = Spring.GetModKeyState()
  mods.alt=alt; mods.ctrl=ctrl; mods.meta=meta; mods.shift=shift;

  return screen0:MouseUp(x,y,button,mods)
end


function widget:MouseMove(x,y,dx,dy,button)
  local alt, ctrl, meta, shift = Spring.GetModKeyState()
  mods.alt=alt; mods.ctrl=ctrl; mods.meta=meta; mods.shift=shift;

  return screen0:MouseMove(x,y,dx,dy,button,mods)
end


function widget:MouseWheel(up,value)
  local x,y = Spring.GetMouseState()
  local alt, ctrl, meta, shift = Spring.GetModKeyState()
  mods.alt=alt; mods.ctrl=ctrl; mods.meta=meta; mods.shift=shift;

  return screen0:MouseWheel(x,y,up,value,mods)
end

-- multi-touch stuff
function widget:AddCursor(x, y, id)
  return screen0:AddCursor(x, y, id)
end

function widget:UpdateCursor(x, y, dx, dy, id)
  return screen0:UpdateCursor(x, y, dx, dy, id)
end

function widget:RemoveCursor(x, y, dx, dy, id)
  return screen0:RemoveCursor(x, y, dx, dy, id)
end

function widget:RefreshCursors(seconds)
  screen0:RefreshCursors(seconds)
end

function widget:ViewResize(vsx, vsy) 
	screen0.width = vsx 
	screen0.height= vsy
end 

widget.TweakIsAbove      = widget.IsAbove
widget.TweakMousePress   = widget.MousePress
widget.TweakMouseRelease = widget.MouseRelease
widget.TweakMouseMove    = widget.MouseMove
widget.TweakMouseWheel   = widget.MouseWheel

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
