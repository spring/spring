Screen = Object:Inherit{
  classname = 'screen',
  x         = 0,
  y         = 0,
  width     = 1e9,
  height    = 1e9,

  preserveChildrenOrder = true,

  activeControl = nil,
  hoveredControl = nil,
  currentTooltip = nil,
  _lastHoveredControl = nil,

  _lastClicked = Spring.GetTimer(),
  _lastClickedX = 0,
  _lastClickedY = 0,
  
  _lastTouched = Spring.GetTimer(),
  _lastTouchedX = 0,
  _lastTouchedY = 0,
  
  activeTouchControllers = {}
}

local this = Screen
local inherited = this.inherited

--//=============================================================================

function Screen:New(obj)
  local vsx,vsy = gl.GetViewSizes()
  obj.width  = vsx
  obj.height = vsy
  obj = inherited.New(self,obj)

  TaskHandler.RequestGlobalDispose(obj)

  return obj
end


function Screen:OnGlobalDispose(obj)
  if (UnlinkSafe(self.activeControl) == obj) then
    self.activeControl = nil
  end

  if (UnlinkSafe(self.hoveredControl) == obj) then
    self.hoveredControl = nil
  end

  if (UnlinkSafe(self._lastHoveredControl) == obj) then
    self._lastHoveredControl = nil
  end
end

--//=============================================================================

--FIXME add new coordspace Device (which does y-invert)

function Screen:ParentToLocal(x,y)
  return x, y
end


function Screen:LocalToParent(x,y)
  return x, y
end


function Screen:LocalToScreen(x,y)
  return x, y
end


function Screen:ScreenToLocal(x,y)
  return x, y
end


function Screen:ScreenToClient(x,y)
  return x, y
end


function Screen:ClientToScreen(x,y)
  return x, y
end

--//=============================================================================

function Screen:IsAbove(x,y,...)
  y = select(2,gl.GetViewSizes()) - y
  local hoveredControl = inherited.IsAbove(self,x,y,...)

  --// tooltip
  if (UnlinkSafe(hoveredControl) ~= UnlinkSafe(self._lastHoveredControl)) then
    self.hoveredControl = MakeWeakLink(hoveredControl)
    if (hoveredControl) then
      local control = hoveredControl
      --// find tooltip in hovered control or its parents
      while (not control.tooltip)and(control.parent) do
        control = control.parent
      end
      self.currentTooltip = control.tooltip
    else
      self.currentTooltip = nil
    end
    self._lastHoveredControl = self.hoveredControl
  elseif (self._lastHoveredControl) then
    self.currentTooltip = self._lastHoveredControl.tooltip
  end

  return (not not hoveredControl)
end


function Screen:MouseDown(x,y,...)
  y = select(2,gl.GetViewSizes()) - y
  local activeControl = inherited.MouseDown(self,x,y,...)
  self.activeControl = MakeWeakLink(activeControl)
  return (not not activeControl)
end


function Screen:MouseUp(x,y,...)
  y = select(2,gl.GetViewSizes()) - y
  local activeControl = Unlink(self.activeControl)
  if activeControl then
    local cx,cy = activeControl:ScreenToLocal(x,y)
    local now = Spring.GetTimer()
    local obj

    local hoveredControl = Unlink(self.hoveredControl)
    if (hoveredControl == activeControl) then
      --//FIXME send this to controls too, when they didn't `return self` in MouseDown!
      if (math.abs(x - self._lastClickedX)<3) and
         (math.abs(y - self._lastClickedY)<3) and
         (Spring.DiffTimers(now,self._lastClicked) < 0.45 ) --FIXME 0.45 := doubleClick time (use spring config?)
      then
        obj = activeControl:MouseDblClick(cx,cy,...)
      end
      if (obj == nil) then
        obj = activeControl:MouseClick(cx,cy,...)
      end
    end
    self._lastClicked = now
    self._lastClickedX = x
    self._lastClickedY = y

    obj = activeControl:MouseUp(cx,cy,...) or obj
    self.activeControl = nil
    return (not not obj)
  else
    return (not not inherited.MouseUp(self,x,y,...))
  end
end


function Screen:MouseMove(x,y,dx,dy,...)
  y = select(2,gl.GetViewSizes()) - y
  local activeControl = Unlink(self.activeControl)
  if activeControl then
    local cx,cy = activeControl:ScreenToLocal(x,y)
    local obj = activeControl:MouseMove(cx,cy,dx,-dy,...)
    if (obj==false) then
      self.activeControl = nil
    elseif (not not obj)and(obj ~= activeControl) then
      self.activeControl = MakeWeakLink(obj)
      return true
    else
      return true
    end
  end

  return (not not inherited.MouseMove(self,x,y,dx,-dy,...))
end


function Screen:MouseWheel(x,y,...)
  y = select(2,gl.GetViewSizes()) - y
  local activeControl = Unlink(self.activeControl)
  if activeControl then
    local cx,cy = activeControl:ScreenToLocal(x,y)
    local obj = activeControl:MouseWheel(cx,cy,...)
    if (obj==false) then
      self.activeControl = nil
    elseif (not not obj)and(obj ~= activeControl) then
      self.activeControl = MakeWeakLink(obj)
      return true
    else
      return true
    end
  end

  return (not not inherited.MouseWheel(self,x,y,...))
end

function Screen:AddCursor(x,y, id)
  local activeControl = inherited.AddCursor(self,x,y,id)
  
  self.activeTouchControllers[id] = MakeWeakLink(activeControl)
  return (not not self.activeTouchControllers[id])
end

function Screen:UpdateCursor(x,y,dx,dy,id)
  
  local activeControl = Unlink(self.activeTouchControllers[id])
  
  if activeControl then
    local cx,cy = activeControl:ScreenToLocal(x,y)
    --Spring.Echo("Screen to Local in Update on " .. activeControl.classname .. " x: " .. x .. " y: " .. y .. " cx: " .. cx .. " cy: " .. cy)
    local obj = activeControl:UpdateCursor(cx,cy,dx,dy,id)
    
    if (obj==false) then
      --Spring.Echo("Deactivating Touch: " .. id)
      self.activeTouchControllers[id] = nil
      activeControl = nil
    elseif (not not obj)and(obj ~= activeControl) then
      --should probably never happen
      self.activeTouchControllers[id] = MakeWeakLink(obj)
      activeControl = self.activeTouchControllers[id]
      cx, cy = activeControl:ScreenToLocal(x,y) 
      return activeControl:CursorEntered(cx, cy, id)
    else
      return true
    end

  end

  if (not activeControl) then
    activeControl = inherited.CursorEntered(self,x,y,id)
    
    if(not not activeControl) then
      --if(activeControl.classname == "button") then
      --  Spring.Echo("Successfully Switched Controls")
      --end
      self.activeTouchControllers[id] = MakeWeakLink(activeControl)
    end
    
    return (not not activeControl)
  end
  
  --shouldn't get here I think
  return (not not inherited.UpdateCursor(self,x,y,dx,dy,id)) 
end

function Screen:RemoveCursor(x, y, dx, dy, id)
  local activeControl = Unlink(self.activeTouchControllers[id])
  
  if activeControl then
    local cx,cy = activeControl:ScreenToLocal(x,y)
    local now = Spring.GetTimer()
    local obj

--    local hoveredControl = Unlink(self.hoveredControl)
--    if (hoveredControl == activeControl) then
      --//FIXME send this to controls too, when they didn't `return self` in MouseDown!
--      if (math.abs(x - self._lastClickedX)<3) and
--         (math.abs(y - self._lastClickedY)<3) and
--         (Spring.DiffTimers(now,self._lastClicked) < 0.45 ) --FIXME 0.45 := doubleClick time (use spring config?)
--      then
--        obj = activeControl:MouseDblClick(cx,cy)
--      end
--      if (obj == nil) then
--        obj = activeControl:MouseClick(cx,cy)
--      end
--    end

--    self._lastClicked = now
--    self._lastClickedX = x
--    self._lastClickedY = y
    
    if (obj == nil) then
      obj = activeControl:TouchTap(cx,cy)
    end
    
    obj = activeControl:RemoveCursor(cx,cy,dx,dy, id) or obj
        
    self.activeTouchControllers[id] = nil
    return (not not obj)
  else
    return (not not inherited.RemoveCursor(self,x,y,dx,dy, id))
  end
end

function Screen:RefreshCursors(seconds)
  local listeningWidgets = {}
  
  for _, ac in ipairs(self.activeTouchControllers) do
    local activeControl = Unlink(ac)
    
    if( not listeningWidgets[activeControl]) then
      listeningWidgets[activeControl] = true
      if( activeControl.RefreshCursor) then
        activeControl:RefreshCursors(seconds)
      end
    end
    
  end
end


--//=============================================================================