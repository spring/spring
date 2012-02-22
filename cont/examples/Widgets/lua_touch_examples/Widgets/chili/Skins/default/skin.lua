--//=============================================================================
--// default

local skin = {
  info = {
    name    = "default",
    version = "0.1",
    author  = "jK",
  }
}

--//=============================================================================
--// Render Helpers

local function _DrawBorder(x,y,w,h,bt,color1,color2)
  gl.Color(color1)
  gl.Vertex(x,     y+h)
  gl.Vertex(x+bt,  y+h-bt)
  gl.Vertex(x,     y)
  gl.Vertex(x+bt,  y)
  gl.Vertex(x+bt,  y)
  gl.Vertex(x+bt,  y+bt)
  gl.Vertex(x+w,   y)
  gl.Vertex(x+w-bt,y+bt)

  gl.Color(color2)
  gl.Vertex(x+w-bt,y+bt)
  gl.Vertex(x+w,   y)
  gl.Vertex(x+w-bt,y+h)
  gl.Vertex(x+w,   y+h)
  gl.Vertex(x+w-bt,y+h-bt)
  gl.Vertex(x+w-bt,y+h)
  gl.Vertex(x+bt,  y+h-bt)
  gl.Vertex(x+bt,  y+h)
  gl.Vertex(x,     y+h)
end


local function _DrawCheck(rect)
  local x,y,w,h = rect[1],rect[2],rect[3],rect[4]
  gl.Vertex(x+w*0.25, y+h*0.5)
  gl.Vertex(x+w*0.125,y+h*0.625)
  gl.Vertex(x+w*0.375,y+h*0.625)
  gl.Vertex(x+w*0.375,y+h*0.875)
  gl.Vertex(x+w*0.75, y+h*0.25)
  gl.Vertex(x+w*0.875,y+h*0.375)
end



local function _DrawDragGrip(obj)
  local x = obj.x + obj.borderThickness + 1
  local y = obj.y + obj.borderThickness + 1
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
  local resizable   = obj.resizable
  if IsTweakMode() then
    resizable   = resizable   or obj.tweakResizable
  end

  if (resizable) then
    local x = obj.x + obj.width - obj.padding[3] --obj.borderThickness - 1
    local y = obj.y + obj.height - obj.padding[4] --obj.borderThickness - 1
    local w = obj.resizeGripSize[1]
    local h = obj.resizeGripSize[2]

    x = x-1
    y = y-1
    gl.Color(1,1,1,0.2)
      gl.Vertex(x - w, y)
      gl.Vertex(x, y - h)

      gl.Vertex(x - math.floor(w*0.66), y)
      gl.Vertex(x, y - math.floor(h*0.66))

      gl.Vertex(x - math.floor(w*0.33), y)
      gl.Vertex(x, y - math.floor(h*0.33))

    x = x+1
    y = y+1
    gl.Color(0.1, 0.1, 0.1, 0.9)
      gl.Vertex(x - w, y)
      gl.Vertex(x, y - h)

      gl.Vertex(x - math.floor(w*0.66), y)
      gl.Vertex(x, y - math.floor(h*0.66))

      gl.Vertex(x - math.floor(w*0.33), y)
      gl.Vertex(x, y - math.floor(h*0.33))
  end
end


--//=============================================================================
--//

function DrawBorder(obj,state)
  local x = obj.x
  local y = obj.y
  local w = obj.width
  local h = obj.height
  local bt = obj.borderThickness

  gl.Color((state=='pressed' and obj.borderColor2) or obj.borderColor1)
  gl.Vertex(x,     y+h)
  gl.Vertex(x+bt,  y+h-bt)
  gl.Vertex(x,     y)
  gl.Vertex(x+bt,  y)
  gl.Vertex(x+bt,  y)
  gl.Vertex(x+bt,  y+bt)
  gl.Vertex(x+w,   y)
  gl.Vertex(x+w-bt,y+bt)

  gl.Color((state=='pressed' and obj.borderColor1) or obj.borderColor2)
  gl.Vertex(x+w-bt,y+bt)
  gl.Vertex(x+w,   y)
  gl.Vertex(x+w-bt,y+h)
  gl.Vertex(x+w,   y+h)
  gl.Vertex(x+w-bt,y+h-bt)
  gl.Vertex(x+w-bt,y+h)
  gl.Vertex(x+bt,  y+h-bt)
  gl.Vertex(x+bt,  y+h)
  gl.Vertex(x,     y+h)
end


function DrawBackground(obj)
  gl.BeginEnd(GL.TRIANGLE_STRIP, _DrawBackground, obj)
end


function _DrawScrollbar(obj, type, x,y,w,h, pos, visiblePercent, state)
  gl.Color(obj.backgroundColor)
  gl.Rect(x,y,x+w,y+h)

  if (type=='horizontal') then
    local gripx,gripw = x+w*pos, w*visiblePercent
    gl.BeginEnd(GL.TRIANGLE_STRIP, _DrawBorder, gripx,y,gripw,h, 1, obj.borderColor1, obj.borderColor2)
  else
    local gripy,griph = y+h*pos, h*visiblePercent
    gl.BeginEnd(GL.TRIANGLE_STRIP, _DrawBorder, x,gripy,w,griph, 1, obj.borderColor1, obj.borderColor2)
  end
end


function _DrawBackground(obj)
  local x = obj.x
  local y = obj.y
  local w = obj.width
  local h = obj.height
	
  gl.Color(obj.backgroundColor)
  gl.Vertex(x,   y)
  gl.Vertex(x,   y+h)
  gl.Vertex(x+w, y)
  gl.Vertex(x+w, y+h)
end


--//=============================================================================
--// Control Renderer

function DrawWindow(obj)
  gl.BeginEnd(GL.TRIANGLE_STRIP, _DrawBackground, obj, obj.state)
  gl.BeginEnd(GL.TRIANGLE_STRIP, DrawBorder, obj, obj.state)
end


function DrawButton(obj)
  gl.BeginEnd(GL.TRIANGLE_STRIP, _DrawBackground, obj, obj.state)
  gl.BeginEnd(GL.TRIANGLE_STRIP, DrawBorder, obj, obj.state)

  if (obj.caption) then
    local x = obj.x
    local y = obj.y
    local w = obj.width
    local h = obj.height

    obj.font:Print(obj.caption, x+w*0.5, y+h*0.5, "center", "center")
  end
end


function DrawPanel(obj)
  gl.BeginEnd(GL.TRIANGLE_STRIP, _DrawBackground, obj, obj.state)
  gl.BeginEnd(GL.TRIANGLE_STRIP, DrawBorder, obj, obj.state)
end


function DrawItemBkGnd(obj,x,y,w,h,state)
  if (state=="selected") then
    gl.Color(0.15,0.15,0.9,1)   
  else
    gl.Color({0.8, 0.8, 1, 0.45})
  end
  gl.Rect(x,y,x+w,y+h)

  gl.BeginEnd(GL.TRIANGLE_STRIP, _DrawBorder, x,y,w,h, 1, obj.borderColor1, obj.borderColor2)
end


function DrawScrollPanel(obj)
  local clientX,clientY,clientWidth,clientHeight = unpack4(obj.clientArea)
  local contX,contY,contWidth,contHeight = unpack4(obj.contentArea)

  gl.PushMatrix()
  gl.Translate(math.floor(obj.x + clientX),math.floor(obj.y + clientY),0)

  if obj._vscrollbar then
    _DrawScrollbar(obj, 'vertical', clientWidth,  0, obj.scrollbarSize, clientHeight,
                        obj.scrollPosY/contHeight, clientHeight/contHeight)
  end
  if obj._hscrollbar then
    _DrawScrollbar(obj, 'horizontal', 0, clientHeight, clientWidth, obj.scrollbarSize, 
                        obj.scrollPosX/contWidth, clientWidth/contWidth)
  end

  gl.PopMatrix()
end


function DrawTrackbar(obj)
  local percent = (obj.value-obj.min)/(obj.max-obj.min)
  local x = obj.x
  local y = obj.y
  local w = obj.width
  local h = obj.height

  gl.Color(0,0,0,1)
  gl.Rect(x,y+h*0.5,x+w,y+h*0.5+1)

  local vc = y+h*0.5 --//verticale center
  local pos = x+percent*w

  gl.Rect(pos-2,vc-h*0.5,pos+2,vc+h*0.5)
end


function DrawCheckbox(obj)
  local vc = obj.height*0.5 --//verticale center
  local tx = 0
  local ty = vc

  gl.PushMatrix()
  gl.Translate(obj.x,obj.y,0)

  obj.font:Print(obj.caption, tx, ty, "left", "center")

  local box  = obj.boxsize
  local rect = {obj.width-box,obj.height*0.5-box*0.5,box,box}

  gl.Color(obj.backgroundColor)
  gl.Rect(rect[1]+1,rect[2]+1,rect[1]+1+rect[3]-2,rect[2]+1+rect[4]-2)

  gl.BeginEnd(GL.TRIANGLE_STRIP, _DrawBorder, rect[1],rect[2],rect[3],rect[4], 1, obj.borderColor1, obj.borderColor2)

  if (obj.checked) then
    gl.BeginEnd(GL.TRIANGLE_STRIP,_DrawCheck,rect)
  end

  gl.PopMatrix()
end


function DrawColorbars(obj)
  gl.PushMatrix()
  gl.Translate(obj.x,obj.y,0)

  local barswidth  = obj.width - (obj.height + 4)

  local color = obj.color
  local step = obj.height/7

  --bars
  local rX1,rY1,rX2,rY2 = 0,0*step,color[1]*barswidth,1*step
  local gX1,gY1,gX2,gY2 = 0,2*step,color[2]*barswidth,3*step
  local bX1,bY1,bX2,bY2 = 0,4*step,color[3]*barswidth,5*step
  local aX1,aY1,aX2,aY2 = 0,6*step,(color[4] or 1)*barswidth,7*step

  gl.Color(1,0,0,1)
  gl.Rect(rX1,rY1,rX2,rY2)

  gl.Color(0,1,0,1)
  gl.Rect(gX1,gY1,gX2,gY2)

  gl.Color(0,0,1,1)
  gl.Rect(bX1,bY1,bX2,bY2)

  gl.Color(1,1,1,1)
  gl.Rect(aX1,aY1,aX2,aY2)

  gl.Color(color)
  gl.Rect(barswidth + 2,obj.height,obj.width - 2,0)

  gl.BeginEnd(GL.TRIANGLE_STRIP, _DrawBorder, barswidth + 2,0,obj.width - barswidth - 4,obj.height, 1, obj.borderColor1,obj.borderColor2)

  gl.PopMatrix()
end


function DrawDragGrip(obj)
  gl.BeginEnd(GL.TRIANGLES, _DrawDragGrip, obj)
end


function DrawResizeGrip(obj)
  gl.BeginEnd(GL.LINES, _DrawResizeGrip, obj)
end


local darkBlue = {0.0,0.0,0.6,0.9}
function DrawTreeviewNode(self)
  if CompareLinks(self.treeview.selected,self) then
    local x = self.x + self.clientArea[1]
    local y = self.y
    local w = self.children[1].width
    local h = self.clientArea[2] + self.children[1].height

    gl.Color(0.1,0.1,1,0.55)
    gl.Rect(x,y,x+w,y+h)
    gl.BeginEnd(GL.TRIANGLE_STRIP, _DrawBorder, x,y,w,h, 1, darkBlue, darkBlue)
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

skin.general = {
  --font        = "FreeSansBold.ttf",
  fontOutline = false,
  fontsize    = 13,
  textColor   = {0,0,0,1},

  --padding         = {5, 5, 5, 5}, --// padding: left, top, right, bottom
  borderThickness = 1.5,
  borderColor1    = {1,1,1,0.6},
  borderColor2    = {0,0,0,0.8},
  backgroundColor = {0.8, 0.8, 1, 0.4},
}

skin.colorbars = {
  DrawControl = DrawColorbars,
}

skin.icons = {
  imageplaceholder = ":cl:placeholder.png",
}

skin.button = {
  DrawControl = DrawButton,
}

skin.checkbox = {
  DrawControl = DrawCheckbox,
}

skin.imagelistview = {
  imageFolder      = "folder.png",
  imageFolderUp    = "folder_up.png",

  DrawItemBackground = DrawItemBkGnd,
}
--[[
skin.imagelistviewitem = {
  padding = {12, 12, 12, 12},

  DrawSelectionItemBkGnd = DrawSelectionItemBkGnd,
}
--]]

skin.panel = {
  DrawControl = DrawPanel,
}

skin.scrollpanel = {
  DrawControl = DrawScrollPanel,
}

skin.trackbar = {
  DrawControl = DrawTrackbar,
}

skin.treeview = {
  ImageExpanded  = ":cl:treeview_node_expanded.png",
  ImageCollapsed = ":cl:treeview_node_collapsed.png",
  treeColor = {0,0,0,0.6},

  minItemHeight = 16,

  DrawNode = DrawTreeviewNode,
  DrawNodeTree = DrawTreeviewNodeTree,
}

skin.window = {
  DrawControl = DrawWindow,
  DrawDragGrip = DrawDragGrip,
  DrawResizeGrip = DrawResizeGrip,
}


skin.control = skin.general


--//=============================================================================
--//

return skin
