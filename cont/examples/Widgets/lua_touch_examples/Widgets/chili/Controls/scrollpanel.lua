--//=============================================================================

ScrollPanel = Control:Inherit{
  classname     = "scrollpanel",
  padding       = {0,0,0,0},
  backgroundColor = {0,0,0,0},
  scrollbarSize = 12,
  scrollPosX    = 0,
  scrollPosY    = 0,
  verticalScrollbar   = true,
  horizontalScrollbar = true,
  verticalSmartScroll = false, -- if control is crolled to bottom, keep scroll when layout changes
}

local this = ScrollPanel
local inherited = this.inherited

--//=============================================================================

function ScrollPanel:SetScrollPos(x,y,inview)
  if (x) then
    if (inview) then
      x = x - self.clientArea[3] * 0.5
    end
    self.scrollPosX = x
    if (self.contentArea) then
      self.scrollPosX = clamp(0, self.contentArea[3] - self.clientArea[3], self.scrollPosX)
    end
  end
  if (y) then
    if (inview) then
      y = y - self.clientArea[4] * 0.5
    end
    self.scrollPosY = y
    if (self.contentArea) then
      self.scrollPosY = clamp(0, self.contentArea[4] - self.clientArea[4], self.scrollPosY)
    end
  end
  self:Invalidate()
end

--//=============================================================================

function ScrollPanel:LocalToClient(x,y)
  local ca = self.clientArea
  return x - ca[1] + self.scrollPosX, y - ca[2] + self.scrollPosY
end


function ScrollPanel:ClientToLocal(x,y)
  local ca = self.clientArea
  return x + ca[1] - self.scrollPosX, y + ca[2] - self.scrollPosY
end


function ScrollPanel:ParentToClient(x,y)
  local ca = self.clientArea
  return x - self.x - ca[1] + self.scrollPosX, y - self.y - ca[2] + self.scrollPosY
end


function ScrollPanel:ClientToParent(x,y)
  local ca = self.clientArea
  return x + self.x + ca[1] - self.scrollPosX, y + self.y + ca[2] - self.scrollPosY
end

--//=============================================================================

function ScrollPanel:_DetermineContentArea()
  local maxRight,maxBottom = self:GetChildrenExtents()

  self.contentArea = {
    0,
    0,
    maxRight,
    maxBottom,
  }

  local contentArea = self.contentArea
  local clientArea = self.clientArea

  if (self.verticalScrollbar) then
    if (contentArea[4]>clientArea[4]) then
      if (not self._vscrollbar) then
        self.padding[3] = self.padding[3] + self.scrollbarSize
      end
      self._vscrollbar = true
    else
      if (self._vscrollbar) then
        self.padding[3] = self.padding[3] - self.scrollbarSize
      end
      self._vscrollbar = false
    end
  end

  if (self.horizontalScrollbar) then
    if (contentArea[3]>clientArea[3]) then
      if (not self._hscrollbar) then
        self.padding[4] = self.padding[4] + self.scrollbarSize
      end
      self._hscrollbar = true
    else
      if (self._hscrollbar) then
        self.padding[4] = self.padding[4] - self.scrollbarSize
      end
      self._hscrollbar = false
    end
  end

  self:UpdateClientArea()

  local contentArea = self.contentArea
  local clientArea = self.clientArea
  if (contentArea[4] < clientArea[4]) then
    contentArea[4] = clientArea[4]
  end
  if (contentArea[3] < clientArea[3]) then
    contentArea[3] = clientArea[3]
  end
end

--//=============================================================================


function ScrollPanel:UpdateLayout()
  --self:_DetermineContentArea()
  self:RealignChildren()
  local before = ((self._vscrollbar and 1) or 0) + ((self._hscrollbar and 2) or 0)
  self:_DetermineContentArea()
  local now = ((self._vscrollbar and 1) or 0) + ((self._hscrollbar and 2) or 0)
  if (before ~= now) then
    self:RealignChildren()
  end

  self.scrollPosX = clamp(0, self.contentArea[3] - self.clientArea[3], self.scrollPosX)
  
  local oldClamp = self.clampY or 0
  self.clampY = self.contentArea[4] - self.clientArea[4]

  if self.verticalSmartScroll and self.scrollPosY >= oldClamp then 
    self.scrollPosY = self.clampY
  else 
    self.scrollPosY = clamp(0, self.clampY, self.scrollPosY)
  end
  
  return true;
end

--//=============================================================================

function ScrollPanel:IsInView(child)
  local clientX,clientY,clientWidth,clientHeight = unpack4(self.clientArea)

  return (child.y - self.scrollPosY <= clientHeight)and
         (child.y + child.height - self.scrollPosY >= 0)and
         (child.x - self.scrollPosX <= clientWidth)and
         (child.x + child.width - self.scrollPosX >= 0);
end

--//=============================================================================

function ScrollPanel:DrawControl()
  --// gets overriden by the skin/theme
end


function ScrollPanel:_DrawInClientArea(fnc,...)
  local clientX,clientY,clientWidth,clientHeight = unpack4(self.clientArea)

  if (self.safeOpengl) then
    local sx,sy = self:LocalToScreen(clientX,clientY)

    --gl.Color(1,0.1,0,0.2)
    --gl.Rect(self.x + clientX, self.y + clientY, self.x + clientX + clientWidth, self.y + clientY + clientHeight)

    sy = select(2,gl.GetViewSizes()) - (sy + clientHeight)
    PushScissor(sx,sy,clientWidth,clientHeight)
  end

  gl.PushMatrix()
  gl.Translate(math.floor(self.x + clientX - self.scrollPosX),math.floor(self.y + clientY - self.scrollPosY),0)
  fnc(...)
  --self:CallChildrenInverseCheckFunc(self.IsInView,...)
  gl.PopMatrix()

  if (self.safeOpengl) then
    PopScissor()
  end
end


--//=============================================================================

function ScrollPanel:IsAboveHScrollbars(x,y)
  if (not self._hscrollbar) then return false end
  local clientArea = self.clientArea
  return x>=clientArea[1] and y>clientArea[2]+clientArea[4] and x<=clientArea[1]+clientArea[3] and y<=clientArea[2]+clientArea[4]+self.scrollbarSize
end


function ScrollPanel:IsAboveVScrollbars(x,y)
  if (not self._vscrollbar) then return false end
  local clientArea = self.clientArea
  return y>=clientArea[2] and x>clientArea[1]+clientArea[3] and y<=clientArea[2]+clientArea[4] and x<=clientArea[1]+clientArea[3]+self.scrollbarSize
end


function ScrollPanel:HitTest(x, y)
  if self:IsAboveVScrollbars(x,y) then
    return self
  end
  if self:IsAboveHScrollbars(x,y) then
    return self
  end

  return inherited.HitTest(self, x, y)
end


function ScrollPanel:MouseDown(x, y, ...)
  if self:IsAboveVScrollbars(x,y) then
    self._vscrolling  = true
    local clientArea = self.clientArea
    local cy = y - clientArea[2]
    self.scrollPosY = (cy/clientArea[4])*self.contentArea[4] - 0.5*clientArea[4]
    self.scrollPosY = clamp(0, self.contentArea[4] - clientArea[4], self.scrollPosY)
    self:Invalidate()
    return self
  end
  if self:IsAboveHScrollbars(x,y) then
    self._hscrolling  = true
    local clientArea = self.clientArea
    local cx = x - clientArea[1]
    self.scrollPosX = (cx/clientArea[3])*self.contentArea[3] - 0.5*clientArea[3]
    self.scrollPosX = clamp(0, self.contentArea[3] - clientArea[3], self.scrollPosX)
    self:Invalidate()
    return self
  end

  return inherited.MouseDown(self, x, y, ...)
end


function ScrollPanel:MouseMove(x, y, dx, dy, ...)
  if self._vscrolling then
    local clientArea = self.clientArea
    local cy = y - clientArea[2]
    self.scrollPosY = (cy/clientArea[4])*self.contentArea[4] - 0.5*clientArea[4]
    self.scrollPosY = clamp(0, self.contentArea[4] - clientArea[4], self.scrollPosY)
    self:Invalidate()
    return self
  end
  if self._hscrolling then
    local clientArea = self.clientArea
    local cx = x - clientArea[1]
    self.scrollPosX = (cx/clientArea[3])*self.contentArea[3] - 0.5*clientArea[3]
    self.scrollPosX = clamp(0, self.contentArea[3] - clientArea[3], self.scrollPosX)
    self:Invalidate()
    return self
  end

  return inherited.MouseMove(self, x, y, dx, dy, ...)
end


function ScrollPanel:MouseUp(x, y, ...)
  if self._vscrolling then
    self._vscrolling = nil
    local clientArea = self.clientArea
    local cy = y - clientArea[2]
    self.scrollPosY = (cy/clientArea[4])*self.contentArea[4] - 0.5*clientArea[4]
    self.scrollPosY = clamp(0, self.contentArea[4] - clientArea[4], self.scrollPosY)
    self:Invalidate()
    return self
  end
  if self._hscrolling then
    self._hscrolling = nil
    local clientArea = self.clientArea
    local cx = x - clientArea[1]
    self.scrollPosX = (cx/clientArea[3])*self.contentArea[3] - 0.5*clientArea[3]
    self.scrollPosX = clamp(0, self.contentArea[3] - clientArea[3], self.scrollPosX)
    self:Invalidate()
    return self
  end

  return inherited.MouseUp(self, x, y, ...)
end


function ScrollPanel:MouseWheel(x, y, up, value, ...)
  if self._vscrollbar and not self.noMouseWheel then
    self.scrollPosY = self.scrollPosY - value*30
    self.scrollPosY = clamp(0, self.contentArea[4] - self.clientArea[4], self.scrollPosY)
    self:Invalidate()
    return self
  end

  return inherited.MouseWheel(self, x, y, up, value, ...)
end
