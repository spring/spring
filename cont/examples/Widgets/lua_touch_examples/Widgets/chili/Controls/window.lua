Window = Control:Inherit{
  classname = 'window',
  resizable = true,
  draggable = true,
  minimizable = false,

  minWidth  = 50,
  minHeight = 50,
  defaultWidth  = 400,
  defaultHeight = 300,
}

local this = Window
local inherited = this.inherited

--//=============================================================================
--[[
function Window:UpdateClientArea()
  this.inherited.UpdateClientArea(self)

  if (not WG['blur_api']) then return end

  if (self.blurId) then
    WG['blur_api'].RemoveBlurRect(self.blurId)
  end

  local screeny = select(2,gl.GetViewSizes()) - self.y

  self.blurId = WG['blur_api'].InsertBlurRect(self.x,screeny,self.x+self.width,screeny-self.height)
end
--]]
--//=============================================================================

function Window:New(obj)
  obj = inherited.New(self,obj)
  obj:BringToFront()
  return obj
end

function Window:DrawControl()
  --// gets overriden by the skin/theme
end

function Window:MouseDown(...)
  self:BringToFront()
  return inherited.MouseDown(self,...)
end

VFS.Include(CHILI_DIRNAME .. "Headers/skinutils.lua")

function Window:TweakDraw()
  gl.Color(0.6,1,0.6,0.65)

  local x = self.x
  local y = self.y
  local w = self.width
  local h = self.height

  if (self.resizable or self.tweakResizable) then
    TextureHandler.LoadTexture(0,"LuaUI/Widgets/chili/Skins/default/tweak_overlay_resizable.png",self)
  else
    TextureHandler.LoadTexture(0,"LuaUI/Widgets/chili/Skins/default/tweak_overlay.png",self)
  end
    local texInfo = gl.TextureInfo("LuaUI/Widgets/chili/Skins/default/tweak_overlay.png") or {xsize=1, ysize=1}
    local tw,th = texInfo.xsize, texInfo.ysize

    gl.BeginEnd(GL.TRIANGLE_STRIP, _DrawTiledTexture, x,y,w,h, 31,31,31,31, tw,th, 0)
  gl.Texture(0,false)
end
