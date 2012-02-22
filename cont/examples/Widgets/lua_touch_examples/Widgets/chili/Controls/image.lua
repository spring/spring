--//=============================================================================

Image = Button:Inherit{
  classname= "image",

  defaultWidth  = 64,
  defaultHeight = 64,
  padding = {0,0,0,0},
  color = {1,1,1,1},

  file  = nil,
  file2 = nil,

  flip  = true;
  flip2 = true;

  keepAspect = true;

  OnClick  = {},
}


local this = Image
local inherited = this.inherited

--//=============================================================================

local function _DrawTextureAspect(x,y,w,h ,tw,th, flipy)
  local twa = w/tw
  local tha = h/th

  local aspect = 1
  if (twa < tha) then
    aspect = twa
    y = y + h*0.5 - th*aspect*0.5
    h = th*aspect
  else
    aspect = tha
    x = x + w*0.5 - tw*aspect*0.5
    w = tw*aspect
  end

  local right  = math.ceil(x+w)
  local bottom = math.ceil(y+h)
  x = math.ceil(x)
  y = math.ceil(y)

  gl.TexRect(x,y,right,bottom,false,flipy)
end

function Image:DrawControl()
  if (not (self.file or self.file2)) then return end
  local c = self.color
  gl.Color(c[1],c[2],c[3],c[4])

  if (self.keepAspect) then
    if (self.file2) then 
      TextureHandler.LoadTexture(0,self.file2,self)
      local texInfo = gl.TextureInfo(self.file2) or {xsize=1, ysize=1}
      local tw,th = texInfo.xsize, texInfo.ysize
      _DrawTextureAspect(self.x,self.y,self.width,self.height, tw,th, self.flip2)
    end 
    if (self.file) then 
      TextureHandler.LoadTexture(0,self.file,self)
      local texInfo = gl.TextureInfo(self.file) or {xsize=1, ysize=1}
      local tw,th = texInfo.xsize, texInfo.ysize
      _DrawTextureAspect(self.x,self.y,self.width,self.height, tw,th, self.flip)
    end
  else
    if (self.file2) then 
      TextureHandler.LoadTexture(0,self.file2,self)
      gl.TexRect(self.x,self.y,self.x+self.width,self.y+self.height,false,self.flip2)
    end 
    if (self.file) then 
      TextureHandler.LoadTexture(0,self.file,self)
      gl.TexRect(self.x,self.y,self.x+self.width,self.y+self.height,false,self.flip)
    end
  end
  gl.Texture(0,false)
end

--//=============================================================================

function Image:IsActive()
  local onclick = self.OnClick
  if (onclick and onclick[1]) then
    return true
  end
end

function Image:HitTest()
  --FIXME check if there are any eventhandlers linked (OnClick,OnMouseUp,...)
  return self:IsActive() and self
end

function Image:MouseDown(...)
  --// we don't use `this` here because it would call the eventhandler of the button class,
  --// which always returns true, but we just want to do so if a calllistener handled the event
  return Control.MouseDown(self, ...) or self:IsActive() and self
end

function Image:MouseUp(...)
  return Control.MouseUp(self, ...) or self:IsActive() and self
end

function Image:MouseClick(...)
  return Control.MouseClick(self, ...) or self:IsActive() and self
end

--//=============================================================================
