--//=============================================================================

Font = Object:Inherit{
  classname     = 'font',

  font          = "FreeSansBold.otf",
  size          = 12,
  outlineWidth  = 3,
  outlineWeight = 3,

  shadow        = false,
  outline       = false,
  color         = {1,1,1,1},
  outlineColor  = {0,0,0,1},
  autoOutlineColor = true,
}

local this = Font
local inherited = this.inherited

--//=============================================================================

function Font:New(obj)
  obj = inherited.New(self,obj)

  --// Load the font
  obj:_LoadFont()

  return obj
end


function Font:Dispose()
  if (not self.disposed) then
    FontHandler.UnloadFont(self._font)  
  end
  inherited.Dispose(self)
end

--//=============================================================================

function Font:_LoadFont()
  local oldfont = self._font
  self._font = FontHandler.LoadFont(self.font,self.size,self.outlineWidth,self.outlineWeight)
  --// do this after LoadFont because it can happen that LoadFont returns the same font again
  --// but if we Unload our old one before, the gc could collect it before, so the engine would have to reload it again
  FontHandler.UnloadFont(oldfont)
end

--//=============================================================================

do
  --// Create some Set... methods (e.g. SetColor, SetSize, SetFont, ...)

  local params = {
    font = true,
    size = true,
    outlineWidth = true,
    outlineWeight = true,
    shadow = false,
    outline = false,
    color = false,
    outlineColor = false,
    autoOutlineColor = false,
  }

  for param,recreateFont in pairs(params) do
    local paramWithUpperCase = param:gsub("^%l", string.upper)
    local funcname = "Set" .. paramWithUpperCase

    Font[funcname] = function(self,value,...)
      local t = type(value)

      if (t == "table") then
        self[param] = table.shallowcopy(value)
      else
        local to = type(self[param])
        if (to == "table") then
          --// this allows :SetColor(r,g,b,a) and :SetColor({r,g,b,a})
          local newtable = {value,...}
          table.merge(newtable,self[param])
          self[param] = newtable
        else
          self[param] = value
        end
      end

      local p = self.parent
      if (recreateFont) then
        self:_LoadFont()
        if (p) then
          p:RequestRealign() 
        end
      else
        if (p) then
          p:Invalidate() 
        end
      end
    end
  end

  params = nil
end

--//=============================================================================

function Font:GetLineHeight(size)
  return self._font.lineheight * (size or self.size)
end

function Font:GetAscenderHeight(size)
  local font = self._font
  return (font.lineheight + font.descender) * (size or self.size)
end

function Font:GetTextWidth(text, size)
  return (self._font and (self._font):GetTextWidth(text) * (size or self.size)) or 0
end

function Font:GetTextHeight(text, size)
  if (not size) then size = self.size end
  local h,descender,numlines = (self._font):GetTextHeight(text)
  return h*size, descender*size, numlines
end

function Font:WrapText(text, width, height, size)
  if (not size) then size = self.size end
  if (height < 1.5 * self._font.lineheight)or(width < size) then
    return text --//workaround for a bug in <=80.5.2
  end
  return (self._font):WrapText(text,width,height,size)
end

--//=============================================================================

function Font.AdjustPosToAlignment(x, y, width, height, align, valign)
  local extra = ''

  --// vertical alignment
  if valign == "center" then
    y     = y + height/2
    extra = 'v'
  elseif valign == "top" then
    extra = 't'
  elseif valign == "bottom" then
    y     = y + height
    extra = 'b'
  else
    --// ascender
    extra = 'a'
  end

  --// horizontal alignment
  if align == "left" then
    --do nothing
  elseif align == "center" then
    x     = x + width/2
    extra = extra .. 'c'
  elseif align == "right" then
    x     = x + width
    extra = extra .. 'r'
  end

  return x,y,extra
end

local AdjustPosToAlignment = Font.AdjustPosToAlignment

local function _GetExtra(align, valign)
  local extra = ''

  --// vertical alignment
  if valign == "center" then
    extra = 'v'
  elseif valign == "top" then
    extra = 't'
  elseif valign == "bottom" then
    extra = 'b'
  else
    --// ascender
    extra = 'a'
  end

  --// horizontal alignment
  if align == "left" then
    --do nothing
  elseif align == "center" then
    extra = extra .. 'c'
  elseif align == "right" then
    extra = extra .. 'r'
  end

  return extra
end

--//=============================================================================

function Font:Draw(text, x, y, align, valign)
  if (not text) then
    return
  end

  local font = self._font

  local extra = _GetExtra(align, valign)
  if self.outline then
	extra = extra .. 'o'
  elseif self.shadow then
	extra = extra .. 's'
  end

  gl.PushMatrix()
    gl.Scale(1,-1,1)
    font:Begin()
      font:SetTextColor(self.color)
      font:SetOutlineColor(self.outlineColor)
      font:SetAutoOutlineColor(self.autoOutlineColor)
        font:Print(text, x, -y, self.size, extra)
    font:End()
  gl.PopMatrix()
end


function Font:DrawInBox(text, x, y, w, h, align, valign)
  if (not text) then
    return
  end

  local font = self._font

  local x,y,extra = AdjustPosToAlignment(x, y, w, h, align, valign)
  
  if self.outline then
	extra = extra .. 'o'
  elseif self.shadow then
	extra = extra .. 's'
  end

  y = y + 1 --// FIXME: if this isn't done some chars as 'R' get truncated at the top

  gl.PushMatrix()
    gl.Scale(1,-1,1)
    font:Begin()
      font:SetTextColor(self.color)
      font:SetOutlineColor(self.outlineColor)
      font:SetAutoOutlineColor(self.autoOutlineColor)
        font:Print(text, x, -y, self.size, extra)
    font:End()
  gl.PopMatrix()
end

Font.Print = Font.Draw
Font.PrintInBox = Font.DrawInBox

--//=============================================================================
