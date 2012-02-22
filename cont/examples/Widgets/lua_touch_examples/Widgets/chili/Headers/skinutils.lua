--//=============================================================================
--//

function _DrawTextureAspect(x,y,w,h ,tw,th)
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

  gl.TexRect(x,y,right,bottom,false,true)
end
local _DrawTextureAspect = _DrawTextureAspect


function _DrawTiledTexture(x,y,w,h, skLeft,skTop,skRight,skBottom, texw,texh, texIndex)
    texIndex = texIndex or 0

    local txLeft   = skLeft/texw
    local txTop    = skTop/texh
    local txRight  = skRight/texw
    local txBottom = skBottom/texh

    --//scale down the texture if we don't have enough space
    local scaleY = h/(skTop+skBottom)
    local scaleX = w/(skLeft+skRight)
    local scale = (scaleX < scaleY) and scaleX or scaleY
    if (scale<1) then
      skTop = skTop * scale
      skBottom = skBottom * scale
      skLeft = skLeft * scale
      skRight = skRight * scale
    end

    --//topleft
    gl.MultiTexCoord(texIndex,0,0)
    gl.Vertex(x,      y)
    gl.MultiTexCoord(texIndex,0,txTop)
    gl.Vertex(x,      y+skTop)
    gl.MultiTexCoord(texIndex,txLeft,0)
    gl.Vertex(x+skLeft, y)
    gl.MultiTexCoord(texIndex,txLeft,txTop)
    gl.Vertex(x+skLeft, y+skTop)

    --//topcenter
    gl.MultiTexCoord(texIndex,1-txRight,0)
    gl.Vertex(x+w-skRight, y)
    gl.MultiTexCoord(texIndex,1-txRight,txTop)
    gl.Vertex(x+w-skRight, y+skTop)

    --//topright
    gl.MultiTexCoord(texIndex,1,0)
    gl.Vertex(x+w,       y)
    gl.MultiTexCoord(texIndex,1,txTop)
    gl.Vertex(x+w,       y+skTop)

    --//right center
    gl.MultiTexCoord(texIndex,1,1-txBottom)
    gl.Vertex(x+w,       y+h-skBottom)    --//degenerate
    gl.MultiTexCoord(texIndex,1-txRight,txTop)
    gl.Vertex(x+w-skRight, y+skTop)
    gl.MultiTexCoord(texIndex,1-txRight,1-txBottom)
    gl.Vertex(x+w-skRight, y+h-skBottom)

    --//background
    gl.MultiTexCoord(texIndex,txLeft,txTop)
    gl.Vertex(x+skLeft,    y+skTop)
    gl.MultiTexCoord(texIndex,txLeft,1-txBottom)
    gl.Vertex(x+skLeft,    y+h-skBottom)

    --//left center
    gl.MultiTexCoord(texIndex,0,txTop)
    gl.Vertex(x,    y+skTop)
    gl.MultiTexCoord(texIndex,0,1-txBottom)
    gl.Vertex(x,    y+h-skBottom)

    --//bottom right
    gl.MultiTexCoord(texIndex,0,1)
    gl.Vertex(x,      y+h)    --//degenerate
    gl.MultiTexCoord(texIndex,txLeft,1-txBottom)
    gl.Vertex(x+skLeft, y+h-skBottom)
    gl.MultiTexCoord(texIndex,txLeft,1)
    gl.Vertex(x+skLeft, y+h)

    --//bottom center
    gl.MultiTexCoord(texIndex,1-txRight,1-txBottom)
    gl.Vertex(x+w-skRight, y+h-skBottom)
    gl.MultiTexCoord(texIndex,1-txRight,1)
    gl.Vertex(x+w-skRight, y+h)

    --//bottom right
    gl.MultiTexCoord(texIndex,1,1-txBottom)
    gl.Vertex(x+w, y+h-skBottom)
    gl.MultiTexCoord(texIndex,1,1)
    gl.Vertex(x+w, y+h)
end
local _DrawTiledTexture = _DrawTiledTexture


function _DrawTiledBorder(x,y,w,h, skLeft,skTop,skRight,skBottom, texw,texh, texIndex)
  texIndex = texIndex or 0

  local txLeft   = skLeft/texw
  local txTop    = skTop/texh
  local txRight  = skRight/texw
  local txBottom = skBottom/texh

  --//scale down the texture if we don't have enough space
  local scaleY = h/(skTop+skBottom)
  local scaleX = w/(skLeft+skRight)
  local scale = (scaleX < scaleY) and scaleX or scaleY
  if (scale<1) then
    skTop = skTop * scale
    skBottom = skBottom * scale
    skLeft = skLeft * scale
    skRight = skRight * scale
  end

  --//topleft
  gl.MultiTexCoord(texIndex,0,0)
  gl.Vertex(x,      y)
  gl.MultiTexCoord(texIndex,0,txTop)
  gl.Vertex(x,      y+skTop)
  gl.MultiTexCoord(texIndex,txLeft,0)
  gl.Vertex(x+skLeft, y)
  gl.MultiTexCoord(texIndex,txLeft,txTop)
  gl.Vertex(x+skLeft, y+skTop)

  --//topcenter
  gl.MultiTexCoord(texIndex,1-txRight,0)
  gl.Vertex(x+w-skRight, y)
  gl.MultiTexCoord(texIndex,1-txRight,txTop)
  gl.Vertex(x+w-skRight, y+skTop)

  --//topright
  gl.MultiTexCoord(texIndex,1,0)
  gl.Vertex(x+w,       y)
  gl.MultiTexCoord(texIndex,1,txTop)
  gl.Vertex(x+w,       y+skTop)

  --//right center
  gl.Vertex(x+w,         y+skTop)    --//degenerate
  gl.MultiTexCoord(texIndex,1-txRight,txTop)
  gl.Vertex(x+w-skRight, y+skTop)
  gl.MultiTexCoord(texIndex,1,1-txBottom)
  gl.Vertex(x+w,         y+h-skBottom)
  gl.MultiTexCoord(texIndex,1-txRight,1-txBottom)
  gl.Vertex(x+w-skRight, y+h-skBottom)

  --//bottom right
  gl.MultiTexCoord(texIndex,1,1)
  gl.Vertex(x+w,         y+h)
  gl.MultiTexCoord(texIndex,1-txRight,1)
  gl.Vertex(x+w-skRight, y+h)

  --//bottom center
  gl.Vertex(x+w-skRight, y+h)    --//degenerate
  gl.MultiTexCoord(texIndex,1-txRight,1-txBottom)
  gl.Vertex(x+w-skRight, y+h-skBottom)
  gl.MultiTexCoord(texIndex,txLeft,1)
  gl.Vertex(x+skLeft,    y+h)
  gl.MultiTexCoord(texIndex,txLeft,1-txBottom)
  gl.Vertex(x+skLeft,    y+h-skBottom)

  --//bottom left
  gl.MultiTexCoord(texIndex,0,1)
  gl.Vertex(x,        y+h)
  gl.MultiTexCoord(texIndex,0,1-txBottom)
  gl.Vertex(x,        y+h-skBottom)

  --//left center
  gl.Vertex(x,        y+h-skBottom)    --//degenerate
  gl.MultiTexCoord(texIndex,0,txTop)
  gl.Vertex(x,        y+skTop)
  gl.MultiTexCoord(texIndex,txLeft,1-txBottom)
  gl.Vertex(x+skLeft, y+h-skBottom)
  gl.MultiTexCoord(texIndex,txLeft,txTop)
  gl.Vertex(x+skLeft, y+skTop)
end
local _DrawTiledBorder = _DrawTiledBorder


local function _DrawDragGrip(obj)
  local x = obj.x + 13
  local y = obj.y + 8
  local w = obj.dragGripSize[1]
  local h = obj.dragGripSize[2]

  gl.Color(0.8,0.8,0.8,0.9)
  gl.Vertex(x, y + h*0.5)
  gl.Vertex(x + w*0.5, y)
  gl.Vertex(x + w*0.5, y + h*0.5)

  gl.Color(0.3,0.3,0.3,0.9)
  gl.Vertex(x + w*0.5, y + h*0.5)
  gl.Vertex(x + w*0.5, y)
  gl.Vertex(x + w, y + h*0.5)

  gl.Vertex(x + w*0.5, y + h)
  gl.Vertex(x, y + h*0.5)
  gl.Vertex(x + w*0.5, y + h*0.5)

  gl.Color(0.1,0.1,0.1,0.9)
  gl.Vertex(x + w*0.5, y + h)
  gl.Vertex(x + w*0.5, y + h*0.5)
  gl.Vertex(x + w, y + h*0.5)
end


local function _DrawResizeGrip(obj)
  if (obj.resizable) then
    local x = obj.x
    local y = obj.y

    local resizeBox = GetRelativeObjBox(obj,obj.boxes.resize)

    x = x-1
    y = y-1
    gl.Color(1,1,1,0.2)
      gl.Vertex(x + resizeBox[1], y + resizeBox[4])
      gl.Vertex(x + resizeBox[3], y + resizeBox[2])

      gl.Vertex(x + resizeBox[1] + math.ceil((resizeBox[3] - resizeBox[1])*0.33), y + resizeBox[4])
      gl.Vertex(x + resizeBox[3], y + resizeBox[2] + math.ceil((resizeBox[4] - resizeBox[2])*0.33))

      gl.Vertex(x + resizeBox[1] + math.ceil((resizeBox[3] - resizeBox[1])*0.66), y + resizeBox[4])
      gl.Vertex(x + resizeBox[3], y + resizeBox[2] + math.ceil((resizeBox[4] - resizeBox[2])*0.66))

    x = x+1
    y = y+1
    gl.Color(0.1, 0.1, 0.1, 0.9)
      gl.Vertex(x + resizeBox[1], y + resizeBox[4])
      gl.Vertex(x + resizeBox[3], y + resizeBox[2])

      gl.Vertex(x + resizeBox[1] + math.ceil((resizeBox[3] - resizeBox[1])*0.33), y + resizeBox[4])
      gl.Vertex(x + resizeBox[3], y + resizeBox[2] + math.ceil((resizeBox[4] - resizeBox[2])*0.33))

      gl.Vertex(x + resizeBox[1] + math.ceil((resizeBox[3] - resizeBox[1])*0.66), y + resizeBox[4])
      gl.Vertex(x + resizeBox[3], y + resizeBox[2] + math.ceil((resizeBox[4] - resizeBox[2])*0.66))
  end
end

--//=============================================================================
--//

function DrawWindow(obj)
  local x = obj.x
  local y = obj.y
  local w = obj.width
  local h = obj.height

  local skLeft,skTop,skRight,skBottom = unpack4(obj.tiles)

  local c = obj.color
  if (c) then
    gl.Color(c)
  else
    gl.Color(1,1,1,1)
  end
  TextureHandler.LoadTexture(0,obj.TileImage,obj)
    local texInfo = gl.TextureInfo(obj.TileImage) or {xsize=1, ysize=1}
    local tw,th = texInfo.xsize, texInfo.ysize

    gl.BeginEnd(GL.TRIANGLE_STRIP, _DrawTiledTexture, x,y,w,h, skLeft,skTop,skRight,skBottom, tw,th)
  gl.Texture(0,false)

  if (obj.caption) then
    obj.font:Print(obj.caption, x+w*0.5, y+9, "center")
  end
end

--//=============================================================================
--//

function DrawButton(obj)
  local x = obj.x
  local y = obj.y
  local w = obj.width
  local h = obj.height

 
  local yOffset = 1.1
  
  local skLeft,skTop,skRight,skBottom = unpack4(obj.tiles)

  if (obj.state=="pressed") then
    gl.Color(mulColor(obj.backgroundColor,0.4))
    obj.BringToFront(obj)
    if(obj.parent) then
      obj.parent.BringToFront(obj.parent)
    end
  else
    gl.Color(obj.backgroundColor)
  end
  TextureHandler.LoadTexture(0,obj.TileImageBK,obj)
    local texInfo = gl.TextureInfo(obj.TileImageBK) or {xsize=1, ysize=1}
    local tw,th = texInfo.xsize, texInfo.ysize

    gl.BeginEnd(GL.TRIANGLE_STRIP, _DrawTiledTexture, x,y,w,h, skLeft,skTop,skRight,skBottom, tw,th, 0)
  --gl.Texture(0,false)

  if (obj.state=="pressed") then
    gl.Color(0.6,0.6,0.6,1)
  else
    gl.Color(1,1,1,1)
  end
  TextureHandler.LoadTexture(0,obj.TileImageFG,obj)
    local texInfo = gl.TextureInfo(obj.TileImageFG) or {xsize=1, ysize=1}
    local tw,th = texInfo.xsize, texInfo.ysize

    gl.BeginEnd(GL.TRIANGLE_STRIP, _DrawTiledTexture, x,y,w,h, skLeft,skTop,skRight,skBottom, tw,th, 0)
  gl.Texture(0,false)

  if(obj.state=="pressed" and obj.showOffset)then
    gl.Scissor(false)
    gl.PushMatrix()
    
    local scale = 1.3
    
    if(obj.offsetScale) then
      scale = obj.offsetScale
    end
    
    local xdif = (x + w / 2)
    local ydif = (y + h / 2)
    
    gl.Translate(0, math.floor(-(h * scale)) ,0.0)
    
    gl.Translate(-xdif * scale, -ydif * scale, 0)
    
    gl.Scale(scale,scale,1)
    
    gl.Translate(xdif / scale, ydif / scale, 0)
    
    --gl.Translate(x / 2, y / 2, 0)
    
    --gl.Translate(w, h, 0)
    
    
    
    obj.safeOpengl = false
    obj.state = "normal"
    obj.DrawControl(obj)
    
    obj._DrawChildrenInClientArea(obj)
    
    obj.state = "pressed"
    obj.safeOpengl = true
    
    gl.PopMatrix()
    gl.Scissor(true)
  end
  
  if (obj.caption) then
    obj.font:Print(obj.caption, x+w*0.5, y+h*0.5, "center", "center")
  end
end

--//=============================================================================
--//

function DrawPanel(obj)
  local x = obj.x
  local y = obj.y
  local w = obj.width
  local h = obj.height

  local skLeft,skTop,skRight,skBottom = unpack4(obj.tiles)

  gl.Color(obj.backgroundColor)
  TextureHandler.LoadTexture(0,obj.TileImageBK,obj)
    local texInfo = gl.TextureInfo(obj.TileImageBK) or {xsize=1, ysize=1}
    local tw,th = texInfo.xsize, texInfo.ysize

    gl.BeginEnd(GL.TRIANGLE_STRIP, _DrawTiledTexture, x,y,w,h, skLeft,skTop,skRight,skBottom, tw,th, 0)
  --gl.Texture(0,false)

  gl.Color(1,1,1,1)
  TextureHandler.LoadTexture(0,obj.TileImageFG,obj)
    local texInfo = gl.TextureInfo(obj.TileImageFG) or {xsize=1, ysize=1}
    local tw,th = texInfo.xsize, texInfo.ysize

    gl.BeginEnd(GL.TRIANGLE_STRIP, _DrawTiledTexture, x,y,w,h, skLeft,skTop,skRight,skBottom, tw,th, 0)
  gl.Texture(0,false)
end

--//=============================================================================
--//

function DrawItemBkGnd(obj,x,y,w,h,state)
  local skLeft,skTop,skRight,skBottom = unpack4(obj.tiles)

  local texInfo = gl.TextureInfo(obj.imageFG) or {xsize=1, ysize=1}
  local tw,th = texInfo.xsize, texInfo.ysize

  if (state=="selected") then
    gl.Color(obj.colorBK_selected)
  else
    gl.Color(obj.colorBK)
  end
  TextureHandler.LoadTexture(0,obj.imageBK,obj)
    gl.BeginEnd(GL.TRIANGLE_STRIP, _DrawTiledTexture, x,y,w,h, skLeft,skTop,skRight,skBottom, tw,th, 0)
  --gl.Texture(0,false)

  if (state=="selected") then
    gl.Color(obj.colorFG_selected)
  else
    gl.Color(obj.colorFG)
  end
  TextureHandler.LoadTexture(0,obj.imageFG,obj)
    gl.BeginEnd(GL.TRIANGLE_STRIP, _DrawTiledTexture, x,y,w,h, skLeft,skTop,skRight,skBottom, tw,th, 0)
  gl.Texture(0,false)
end

--//=============================================================================
--//

function DrawScrollPanelBorder(self)
  local clientX,clientY,clientWidth,clientHeight = unpack4(self.clientArea)
  local contX,contY,contWidth,contHeight = unpack4(self.contentArea)

  
  gl.Color(self.backgroundColor)

  do
      TextureHandler.LoadTexture(0,self.BorderTileImage,self)
      local texInfo = gl.TextureInfo(self.BorderTileImage) or {xsize=1, ysize=1}
      local tw,th = texInfo.xsize, texInfo.ysize

      local skLeft,skTop,skRight,skBottom = unpack4(self.bordertiles)

      local width = self.width
      local height = self.height
      if (self._vscrollbar) then
        width = clientWidth + self.padding[1] - 1
      end
      if (self._hscrollbar) then
        height = clientHeight + self.padding[2] - 1
      end

      gl.BeginEnd(GL.TRIANGLE_STRIP, _DrawTiledBorder, self.x,self.y,width,height, skLeft,skTop,skRight,skBottom, tw,th, 0)
      gl.Texture(0,false)
  end
end

--//=============================================================================
--//

function DrawScrollPanel(obj)
  local clientX,clientY,clientWidth,clientHeight = unpack4(obj.clientArea)
  local contX,contY,contWidth,contHeight = unpack4(obj.contentArea)

  gl.Color(obj.backgroundColor)

  if (obj.BackgroundTileImage) then
      TextureHandler.LoadTexture(0,obj.BackgroundTileImage,obj)
      local texInfo = gl.TextureInfo(obj.BackgroundTileImage) or {xsize=1, ysize=1}
      local tw,th = texInfo.xsize, texInfo.ysize

      local skLeft,skTop,skRight,skBottom = unpack4(obj.bkgndtiles)

      local width = obj.width
      local height = obj.height
      if (obj._vscrollbar) then
        width = clientWidth + obj.padding[1] - 1
      end
      if (obj._hscrollbar) then
        height = clientHeight + obj.padding[2] - 1
      end

      gl.BeginEnd(GL.TRIANGLE_STRIP, _DrawTiledTexture, obj.x,obj.y,width,height, skLeft,skTop,skRight,skBottom, tw,th, 0)
      gl.Texture(0,false)
  end
  
  gl.Color(1,1,1,1)

  if obj._vscrollbar then
    local x = obj.x + clientX + clientWidth
    local y = obj.y --+ clientY
    local w = obj.scrollbarSize
    local h = obj.height

    local skLeft,skTop,skRight,skBottom = unpack4(obj.tiles)

    TextureHandler.LoadTexture(0,obj.TileImage,obj)
      local texInfo = gl.TextureInfo(obj.TileImage) or {xsize=1, ysize=1}
      local tw,th = texInfo.xsize, texInfo.ysize

      gl.BeginEnd(GL.TRIANGLE_STRIP, _DrawTiledTexture, x,y,w,h, skLeft,skTop,skRight,skBottom, tw,th, 0)
    --gl.Texture(0,false)

    if obj._vscrolling then
      gl.Color(obj.KnobColorSelected)
    end

    TextureHandler.LoadTexture(0,obj.KnobTileImage,obj)
      texInfo = gl.TextureInfo(obj.KnobTileImage) or {xsize=1, ysize=1}
      tw,th = texInfo.xsize, texInfo.ysize

      skLeft,skTop,skRight,skBottom = unpack4(obj.KnobTiles)

      local pos = obj.scrollPosY/contHeight
      local visible = clientHeight/contHeight
      local gripy = y + clientHeight * pos
      local griph = clientHeight * visible
      gl.BeginEnd(GL.TRIANGLE_STRIP, _DrawTiledTexture, x,gripy,obj.scrollbarSize,griph, skLeft,skTop,skRight,skBottom, tw,th, 0)
    --gl.Texture(0,false)

    gl.Color(1,1,1,1)
  end

  if obj._hscrollbar then
    local x = obj.x
    local y = obj.y + clientY + clientHeight
    local w = obj.width
    local h = obj.scrollbarSize

    local skLeft,skTop,skRight,skBottom = unpack4(obj.htiles)

    TextureHandler.LoadTexture(0,obj.HTileImage,obj)
      local texInfo = gl.TextureInfo(obj.HTileImage) or {xsize=1, ysize=1}
      local tw,th = texInfo.xsize, texInfo.ysize

      gl.BeginEnd(GL.TRIANGLE_STRIP, _DrawTiledTexture, x,y,w,h, skLeft,skTop,skRight,skBottom, tw,th, 0)
    --gl.Texture(0,false)

    if obj._hscrolling then
      gl.Color(obj.KnobColorSelected)
    end

    TextureHandler.LoadTexture(0,obj.HKnobTileImage,obj)
      texInfo = gl.TextureInfo(obj.HKnobTileImage) or {xsize=1, ysize=1}
      tw,th = texInfo.xsize, texInfo.ysize

      skLeft,skTop,skRight,skBottom = unpack4(obj.HKnobTiles)

      local pos = obj.scrollPosX/contWidth
      local visible = clientWidth/contWidth
      local gripx = x + clientWidth * pos
      local gripw = clientWidth * visible
      gl.BeginEnd(GL.TRIANGLE_STRIP, _DrawTiledTexture, gripx,y,gripw,obj.scrollbarSize, skLeft,skTop,skRight,skBottom, tw,th, 0)
    --gl.Texture(0,false)
  end

  gl.Texture(0,false)
end

--//=============================================================================
--//

function DrawCheckbox(obj)
  local boxSize = obj.boxsize
  local x = obj.x + obj.width      - boxSize
  local y = obj.y + obj.height*0.5 - boxSize*0.5
  local w = boxSize
  local h = boxSize

  local skLeft,skTop,skRight,skBottom = unpack4(obj.tiles)


  gl.Color(1,1,1,1)
  TextureHandler.LoadTexture(0,obj.TileImageBK,obj)

  local texInfo = gl.TextureInfo(obj.TileImageBK) or {xsize=1, ysize=1}
  local tw,th = texInfo.xsize, texInfo.ysize

  
    gl.BeginEnd(GL.TRIANGLE_STRIP, _DrawTiledTexture, x,y,w,h, skLeft,skTop,skRight,skBottom, tw,th, 0)
  --gl.Texture(0,false)

  if (obj.checked) then
    TextureHandler.LoadTexture(0,obj.TileImageFG,obj)
      gl.BeginEnd(GL.TRIANGLE_STRIP, _DrawTiledTexture, x,y,w,h, skLeft,skTop,skRight,skBottom, tw,th, 0)
  end
  gl.Texture(0,false)

  if (obj.caption) then
    local vc = obj.height*0.5 --//verticale center
    local tx = 0
    local ty = vc

    obj.font:Print(obj.caption, obj.x + tx, obj.y + ty, nil, "center")
  end
end

--//=============================================================================
--//

function DrawProgressbar(obj)
  local x = obj.x
  local y = obj.y
  local w = obj.width
  local h = obj.height

  local percent = (obj.value-obj.min)/(obj.max-obj.min)

  local skLeft,skTop,skRight,skBottom = unpack4(obj.tiles)

  gl.Color(1,1,1,1)
  if not obj.noSkin then
    TextureHandler.LoadTexture(0,obj.TileImageBK,obj)
    local texInfo = gl.TextureInfo(obj.TileImageBK) or {xsize=1, ysize=1}
    local tw,th = texInfo.xsize, texInfo.ysize

    gl.BeginEnd(GL.TRIANGLE_STRIP, _DrawTiledTexture, x,y,w,h, skLeft,skTop,skRight,skBottom, tw,th, 0)
    --gl.Texture(0,false)
  end

  gl.Color(obj.color)
  TextureHandler.LoadTexture(0,obj.TileImageFG,obj)
    local texInfo = gl.TextureInfo(obj.TileImageFG) or {xsize=1, ysize=1}
    local tw,th = texInfo.xsize, texInfo.ysize

    gl.ClipPlane(1, -1,0,0, x+w*percent)
    gl.BeginEnd(GL.TRIANGLE_STRIP, _DrawTiledTexture, x,y,w,h, skLeft,skTop,skRight,skBottom, tw,th, 0)
    gl.ClipPlane(1, false)
  gl.Texture(0,false)

  if (obj.caption) then
    (obj.font):Print(obj.caption, x+w*0.5, y+h*0.5, "center", "center")
  end
end

--//=============================================================================
--//

function DrawTrackbar(self)
  local percent = self:_GetPercent()
  local x = self.x
  local y = self.y
  local w = self.width
  local h = self.height

  local skLeft,skTop,skRight,skBottom = unpack4(self.tiles)
  local pdLeft,pdTop,pdRight,pdBottom = unpack4(self.hitpadding)

  gl.Color(1,1,1,1)
  if not self.noDrawBar then
    TextureHandler.LoadTexture(0,self.TileImage,self)
    local texInfo = gl.TextureInfo(self.TileImage) or {xsize=1, ysize=1}
    local tw,th = texInfo.xsize, texInfo.ysize
    gl.BeginEnd(GL.TRIANGLE_STRIP, _DrawTiledTexture, x,y,w,h, skLeft,skTop,skRight,skBottom, tw,th, 0)
  end
    
  if not self.noDrawStep then
    TextureHandler.LoadTexture(0,self.StepImage,self)
    local texInfo = gl.TextureInfo(self.StepImage) or {xsize=1, ysize=1}
    local tw,th = texInfo.xsize, texInfo.ysize

    --// scale the thumb down if we don't have enough space
    if (th>h) then
      tw = math.ceil(tw*(h/th))
      th = h
    end
    if (tw>w) then
      th = math.ceil(th*(w/tw))
      tw = w
    end

    local barWidth = w - (pdLeft + pdRight)
    local stepWidth = barWidth / ((self.max - self.min)/self.step)

    if ((self.max - self.min)/self.step)<20 then
      local newStepWidth = stepWidth
      if (newStepWidth<20) then
        newStepWidth = stepWidth*2
      end
      if (newStepWidth<20) then
        newStepWidth = stepWidth*5
      end
      if (newStepWidth<20) then
        newStepWidth = stepWidth*10
      end
      stepWidth = newStepWidth

      local my = y+h*0.5
      local mx = x+pdLeft+stepWidth
      while (mx<(x+pdLeft+barWidth)) do
        gl.TexRect(math.ceil(mx-tw*0.5),math.ceil(my-th*0.5),math.ceil(mx+tw*0.5),math.ceil(my+th*0.5),false,true)
        mx = mx+stepWidth
      end
    end
  end

  TextureHandler.LoadTexture(0,self.ThumbImage,self)
    local texInfo = gl.TextureInfo(self.ThumbImage) or {xsize=1, ysize=1}
    local tw,th = texInfo.xsize, texInfo.ysize

    --// scale the thumb down if we don't have enough space
    if (th>h) then
      tw = math.ceil(tw*(h/th))
      th = h
    end
    if (tw>w) then
      th = math.ceil(th*(w/tw))
      tw = w
    end

    local barWidth = w - (pdLeft + pdRight)
    local mx = x+pdLeft+barWidth*percent
    local my = y+h*0.5
    gl.TexRect(math.ceil(mx-tw*0.5),math.ceil(my-th*0.5),math.ceil(mx+tw*0.5),math.ceil(my+th*0.5),false,true)

  gl.Texture(0,false)
end

--//=============================================================================
--//

function DrawTreeviewNode(self)
  if CompareLinks(self.treeview.selected,self) then
    local x = self.x + self.clientArea[1]
    local y = self.y
    local w = self.children[1].width
    local h = self.clientArea[2] + self.children[1].height

    local skLeft,skTop,skRight,skBottom = unpack4(self.treeview.tiles)

    gl.Color(1,1,1,1)
    TextureHandler.LoadTexture(0,self.treeview.ImageNodeSelected,self)
      local texInfo = gl.TextureInfo(self.treeview.ImageNodeSelected) or {xsize=1, ysize=1}
      local tw,th = texInfo.xsize, texInfo.ysize

      gl.BeginEnd(GL.TRIANGLE_STRIP, _DrawTiledTexture, x,y,w,h, skLeft,skTop,skRight,skBottom, tw,th, 0)
    gl.Texture(0,false)
  end
end


local function _DrawLineV(x, y1, y2, width, next_func, ...)
  gl.Vertex(x-width*0.5, y1)
  gl.Vertex(x+width*0.5, y1)
  gl.Vertex(x-width*0.5, y2)

  gl.Vertex(x+width*0.5, y1)
  gl.Vertex(x-width*0.5, y2)
  gl.Vertex(x+width*0.5, y2)

  if (next_func) then
    next_func(...)
  end
end


local function _DrawLineH(x1, x2, y, width, next_func, ...)
  gl.Vertex(x1, y-width*0.5)
  gl.Vertex(x1, y+width*0.5)
  gl.Vertex(x2, y-width*0.5)

  gl.Vertex(x1, y+width*0.5)
  gl.Vertex(x2, y-width*0.5)
  gl.Vertex(x2, y+width*0.5)

  if (next_func) then
    next_func(...)
  end
end


function DrawTreeviewNodeTree(self)
  local x1 = self.x + math.ceil(self.padding[1]*0.5)
  local x2 = self.x + self.padding[1]
  local y1 = self.y
  local y2 = self.y + self.height
  local y3 = self.y + self.padding[2] + math.ceil(self.children[1].height*0.5)

  if (self.parent)and(CompareLinks(self,self.parent.nodes[#self.parent.nodes])) then
    y2 = y3
  end

  gl.Color(self.treeview.treeColor)
  gl.BeginEnd(GL.TRIANGLES, _DrawLineV, x1-0.5, y1, y2, 1, _DrawLineH, x1, x2, y3-0.5, 1)

  if (not self.nodes[1]) then
    return
  end

  gl.Color(1,1,1,1)
  local image = self.ImageExpanded or self.treeview.ImageExpanded
  if (not self.expanded) then
    image = self.ImageCollapsed or self.treeview.ImageCollapsed
  end

  TextureHandler.LoadTexture(0, image, self)
  local texInfo = gl.TextureInfo(image) or {xsize=1, ysize=1}
  local tw,th = texInfo.xsize, texInfo.ysize

  _DrawTextureAspect(self.x,self.y,math.ceil(self.padding[1]),math.ceil(self.children[1].height) ,tw,th)
  gl.Texture(0,false)
end


--//=============================================================================
--// 

function DrawDragGrip(obj)
  gl.BeginEnd(GL.TRIANGLES, _DrawDragGrip, obj)
end


function DrawResizeGrip(obj)
  gl.BeginEnd(GL.LINES, _DrawResizeGrip, obj)
end

--//=============================================================================
--// HitTest helpers

function _ProcessRelative(code, total)
  --// FIXME: duplicated in control.lua!
  if (type(code) == "string") then
    local percent = tonumber(code:sub(1,-2)) or 0
    if (percent<0) then
      percent = 0
    elseif (percent>100) then
      percent = 100
    end
    return math.floor(total * percent)
  elseif (code<0) then
    return math.floor(total + code)
  else
    return math.floor(code)
  end
end


function GetRelativeObjBox(obj,boxrelative)
  return {
    _ProcessRelative(boxrelative[1], obj.width),
    _ProcessRelative(boxrelative[2], obj.height),
    _ProcessRelative(boxrelative[3], obj.width),
    _ProcessRelative(boxrelative[4], obj.height)
  }
end


--//=============================================================================
--//

function NCHitTestWithPadding(obj,mx,my)
  local hp = obj.hitpadding
  local x = hp[1]
  local y = hp[2]
  local w = obj.width - hp[1] - hp[3]
  local h = obj.height - hp[2] - hp[4]

  --// early out
  if (not InRect({x,y,w,h},mx,my)) then
    return false
  end

  local resizable   = obj.resizable
  local draggable   = obj.draggable
  local dragUseGrip = obj.dragUseGrip
  if IsTweakMode() then
    resizable   = resizable  or obj.tweakResizable
    draggable   = draggable  or obj.tweakDraggable
    dragUseGrip = draggable and obj.tweakDragUseGrip
  end

  if (resizable) then
    local resizeBox = GetRelativeObjBox(obj,obj.boxes.resize)
    if (InRect(resizeBox,mx,my)) then
      return obj
    end
  end

  if (dragUseGrip) then
    local dragBox = GetRelativeObjBox(obj,obj.boxes.drag)
    if (InRect(dragBox,mx,my)) then
      return obj
    end
  elseif (draggable) then
    return obj
  end
end

function WindowNCMouseDown(obj,x,y)
  local resizable   = obj.resizable
  local draggable   = obj.draggable
  local dragUseGrip = obj.dragUseGrip
  if IsTweakMode() then
    resizable   = resizable  or obj.tweakResizable
    draggable   = draggable  or obj.tweakDraggable
    dragUseGrip = draggable and obj.tweakDragUseGrip
  end

  if (resizable) then
    local resizeBox = GetRelativeObjBox(obj,obj.boxes.resize)
    if (InRect(resizeBox,x,y)) then
      obj:StartResizing(x,y)
      return obj
    end
  end

  if (dragUseGrip) then
    local dragBox = GetRelativeObjBox(obj,obj.boxes.drag)
    if (InRect(dragBox,x,y)) then
      obj:StartDragging()
      return obj
    end
  end
end

function WindowNCMouseDownPostChildren(obj,x,y)
  local draggable   = obj.draggable
  local dragUseGrip = obj.dragUseGrip
  if IsTweakMode() then
    draggable   = draggable  or obj.tweakDraggable
    dragUseGrip = draggable and obj.tweakDragUseGrip
  end

  if (draggable and not dragUseGrip) then
    obj:StartDragging()
    return obj
  end
end

--//=============================================================================
