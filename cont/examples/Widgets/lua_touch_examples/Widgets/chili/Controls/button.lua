--//=============================================================================

Button = Control:Inherit{
  classname= "button",
  caption  = 'button',
  defaultWidth  = 70,
  defaultHeight = 20,
  showOffset = true,
}

local this = Button
local inherited = this.inherited

--//=============================================================================

function Button:SetCaption(caption)
  self.caption = caption
  self:Invalidate()
end

--//=============================================================================

function Button:DrawControl()
  --// gets overriden by the skin/theme
end


--//=============================================================================

function Button:HitTest(x,y)
  return self
end

function Button:MouseDown(...)
  self._down = true
  self.state = 'pressed'
  inherited.MouseDown(self, ...)
  self:Invalidate()
  return self
end

function Button:AddCursor(...)
  if(not self._down) then
    self._down = true
    self.state = 'pressed'
    inherited.AddCursor(self, ...)
    self:Invalidate()
    return self
  end
end

function Button:CursorEntered(...)
  if(not self._down) then
    self._down = true
    self.state = 'pressed'
    inherited.CursorEntered(self, ...)
    self:Invalidate()
    return self
  end
end


function Button:MouseUp(...)
  if (self._down) then
    self._down = false
    self.state = 'normal'
    inherited.MouseUp(self, ...)
    self:Invalidate()
    return self
  end
end

function Button:RemoveCursor(...)
  if (self._down) then
    self._down = false
    self.state = 'normal'
    inherited.RemoveCursor(self, ...)
    self:Invalidate()
    return self
  end
end

function Button:CursorExited(...)
  if (self._down) then
    self._down = false
    self.state = 'normal'
    inherited.CursorExited(self, ...)
    self:Invalidate()
    return self
  end
end


--//=============================================================================