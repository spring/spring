--//=============================================================================

Object = {
  classname = 'object',
  --x         = 0,
  --y         = 0,
  --width     = 10,
  --height    = 10,
  defaultWidth  = 10,
  defaultHeight = 10,

  preserveChildrenOrder = false, --// if false adding/removing children is much faster, but also the order (in the .children array) isn't reliable anymore

  children    = {},
  childrenByName = CreateWeakTable(),

  OnDispose    = {},
  OnClick      = {},
  OnDblClick   = {},
  OnMouseDown  = {},
  OnMouseUp    = {},
  OnMouseMove  = {},
  OnMouseWheel = {},

  OnAddCursor = {},
  OnUpdateCursor = {},
  OnRemoveCursor = {},
  OnRefreshCursors = {},
  OnTouchTap = {},
  OnCursorEntered = {},
  OnCursorExited = {},

  disableChildrenHitTest = false, --// if set childrens are not clickable/draggable etc - their mouse events are not processed
}

do
  local __lowerkeys = {}
  Object.__lowerkeys = __lowerkeys
  for i,v in pairs(Object) do
    if (type(i)=="string") then
      __lowerkeys[i:lower()] = i
    end
  end
end

local this = Object
local inherited = this.inherited

--//=============================================================================
--// used to generate unique objects names

local cic = {} 
local function GetUniqueId(classname)
  local ci = cic[classname] or 0
  cic[classname] = ci + 1
  return ci
end

--//=============================================================================
function Object:New(obj)
  obj = obj or {}

  --// check if the user made some lower-/uppercase failures
  for i,v in pairs(obj) do
    if (not self[i])and(isstring(i)) then
      local correctName = self.__lowerkeys[i:lower()]
      if (correctName)and(obj[correctName] == nil) then
        obj[correctName] = v
      end
    end
  end

  --// give name
  if (not obj.name) then
    obj.name = self.classname .. GetUniqueId(self.classname)
  end

  --// make an instance
  for i,v in pairs(self) do --// `self` means the class here and not the instance!
    if (i ~= "inherited") then
      local t = type(v)
      local ot = type(obj[i])
      if (t=="table")or(t=="metatable") then
        if (ot == "nil") then
          obj[i] = {};
          ot = "table";
        end
        if (ot ~= "table")and(ot ~= "metatable") then
          Spring.Echo("Chili: " .. obj.name .. ": Wrong param type given to " .. i .. ": got " .. ot .. " expected table.")
          obj[i] = {}
        end

        table.merge(obj[i],v)
        if (t=="metatable") then
          setmetatable(obj[i], getmetatable(v))
        end
      elseif (ot == "nil") then
        obj[i] = v
      end
    end
  end
  setmetatable(obj,{__index = self})

  --// auto dispose remaining Dlists etc. when garbage collector frees this object
  local hobj = MakeHardLink(obj,function() obj:Dispose(); obj=nil; end)

  --// handle children & parent
  local parent = obj.parent
  if (parent) then
    obj.parent = nil
    --// note: we are using the hardlink here,
    --//       else the link could get gc'ed and dispose our object
    parent:AddChild(hobj)
  end
  local cn = obj.children
  obj.children = {}
  for i=1,#cn do
    obj:AddChild(cn[i],true)
  end

  --// link some general events as Update()
  --TaskHandler.AddObject(obj)

  --// sets obj._widget
  DebugHandler:RegisterObject(obj)

  return hobj
end


-- calling this releases unmanaged resources like display lists and disposes of the object
-- children are disposed too
-- todo: use scream, in case the user forgets
-- nil -> nil
function Object:Dispose()
  if (not self.disposed) then
    self:CallListeners(self.OnDispose)

    self.disposed = true

    TaskHandler.RemoveObject(self)
    --DebugHandler:UnregisterObject(self) --// not needed

    if (UnlinkSafe(self.parent)) then
      self.parent:RemoveChild(self)
    end
    self.parent = nil

    local children = self.children
    local cn = #children
    for i=cn,1,-1 do
      local child = children[i]
      --if (child.parent == self) then
        child:SetParent(nil)
      --end
    end
    self.children = {}
  end
end


function Object:Clone()
  local newinst = {}
   -- FIXME
  return newinst
end


function Object:Inherit(class)
  class.inherited = self

  for i,v in pairs(self) do
    if (class[i] == nil)and(i ~= "inherited")and(i ~= "__lowerkeys") then
      t = type(v)
      if (t == "table") --[[or(t=="metatable")--]] then
        class[i] = table.shallowcopy(v)
      else
        class[i] = v
      end
    end
  end

  local __lowerkeys = {}
  class.__lowerkeys = __lowerkeys
  for i,v in pairs(class) do
    if (type(i)=="string") then
      __lowerkeys[i:lower()] = i
    end
  end

  --setmetatable(class,{__index=self})

  return class
end

--//=============================================================================

function Object:SetParent(obj)
  obj = UnlinkSafe(obj)
  local typ = type(obj)

  if (typ ~= "table") then
    self.parent = nil
    return
  end

  self.parent = MakeWeakLink(obj, self.parent)

  self:Invalidate()
end


function Object:AddChild(obj, dontUpdate)
  --FIXME cause of performance reasons it would be usefull to use the direct object, but then we need to cache the link somewhere to avoid the auto calling of dispose
  local objDirect = UnlinkSafe(obj)

  if (self.children[objDirect]) then
    --if (self.debug) then
      Spring.Echo("Chili: tried to add the same child multiple times (".. (obj.name or "") ..")")
    --end
    return
  end


  if (obj.name) then
    if (self.childrenByName[obj.name]) then
      error(("Chili: There is already a control with the name `%s` in `%s`!"):format(obj.name, self.name))
      return
    end
    self.childrenByName[obj.name] = objDirect
  end

  if Unlink(obj.parent) then
    obj.parent:RemoveChild(obj)
  end
  obj:SetParent(self)

  local children = self.children
  local i = #children+1
  children[i] = objDirect
  children[obj] = i --FIXME (unused/unuseful?)
  children[objDirect] = i
  self:Invalidate()
end


function Object:RemoveChild(child)
  if CompareLinks(child.parent,self) then
    child:SetParent(nil)
  end

  if (child.name) then
    self.childrenByName[child.name] = nil
  end

  local childDirect = UnlinkSafe(child)

  local children = self.children
  local cn = #children
  for i=1,cn do
    if CompareLinks(childDirect,children[i]) then
      if (self.preserveChildrenOrder) then
        --// slow
        table.remove(children, i)
      else
        --// fast
        children[i] = children[cn]
        children[cn] = nil
      end

      children[child] = nil --FIXME (unused/unuseful?)
      children[childDirect] = nil

      self:Invalidate()
      return true
    end
  end
  return false
end


function Object:ClearChildren()
  self:CallChildrenInverse("Dispose") --FIXME instead of disposing perhaps just unlink from parent?
  self.children = {}
end


function Object:IsEmpty()
  return (not self.children[1])
end

--//=============================================================================

function Object:SetLayer(child,layer)
  child = UnlinkSafe(child)
  local children = self.children

  --// it isn't at the same pos anymore, search it!
  for i=1,#children do
    if (UnlinkSafe(children[i]) == child) then
      table.remove(children,i)
      break
    end
  end
  table.insert(children,layer,child)
  self:Invalidate()
end


function Object:BringToFront()
  if (self.parent) then
    (self.parent):SetLayer(self,1)
  end
end

--//=============================================================================

function Object:InheritsFrom(classname)
  if (self.classname == classname) then
    return true
  elseif not self.inherited then
    return false
  else
    return self.inherited.InheritsFrom(self.inherited,classname)
  end
end

--//=============================================================================

function Object:GetChildByName(name)
  local cn = self.children
  for i=1,#cn do
    if (name == cn[i].name) then
      return cn[i]
    end
  end
end

--// Backward-Compability
Object.GetChild = Object.GetChildByName


--// Resursive search to find an object by its name
function Object:GetObjectByName(name)
  local cn = self.children
  for i=1,#cn do
    local c = cn[i]
    if (name == c.name) then
      return c
    else
      local result = c:GetObjectByName(name)
      if (result) then
        return result
      end
    end
  end
end


--// Climbs the family tree and returns the first parent that satisfies a 
--// predicate function or inherites the given class.
--// Returns nil if not found.
function Object:FindParent(predicate)
  if not self.parent then
    return -- not parent with such class name found, return nil
  elseif (type(predicate) == "string" and (self.parent):InheritsFrom(predicate)) or
         (type(predicate) == "function" and predicate(self.parent)) then 
    return self.parent
  else
    return self.parent:FindParent(predicate)
  end
end


function Object:IsDescendantOf(object, _already_unlinked)
  if (not _already_unlinked) then
    object = UnlinkSafe(object)
  end
  if (UnlinkSafe(self) == object) then
    return true
  end
  if (self.parent) then
    return (self.parent):IsDescendantOf(object, true)
  end
  return false
end


function Object:IsAncestorOf(object, _level, _already_unlinked)
  _level = _level or 1

  if (not _already_unlinked) then
    object = UnlinkSafe(object)
  end

  local children = self.children

  for i=1,#children do
    if (children[i] == object) then
      return true, _level
    end
  end

  _level = _level + 1
  for i=1,#children do
    local c = children[i]
    local res,lvl = c:IsAncestorOf(object, _level, true)
    if (res) then
      return true, lvl
    end
  end

  return false
end

--//=============================================================================

function Object:CallListeners(listeners, ...)
  for i=1,#listeners do
    local eventListener = listeners[i]
    if eventListener(self, ...) then
      return true
    end
  end
end


function Object:CallListenersInverse(listeners, ...)
  for i=#listeners,1,-1 do
    local eventListener = listeners[i]
    if eventListener(self, ...) then
      return true
    end
  end
end


function Object:CallChildren(eventname, ...)
  local children = self.children
  for i=1,#children do
    local child = children[i]
    if (child) then
      local obj = child[eventname](child, ...)
      if (obj) then
        return obj
      end
    end
  end
end


function Object:CallChildrenInverse(eventname, ...)
  local children = self.children
  for i=#children,1,-1 do
    local child = children[i]
    if (child) then
      local obj = child[eventname](child, ...)
      if (obj) then
        return obj
      end
    end
  end
end


function Object:CallChildrenInverseCheckFunc(checkfunc,eventname, ...)
  local children = self.children
  for i=#children,1,-1 do
    local child = children[i]
    if (child)and(checkfunc(self,child)) then
      local obj = child[eventname](child, ...)
      if (obj) then
        return obj
      end
    end
  end
end


local function InLocalRect(cx,cy,w,h)
  return (cx>=0)and(cy>=0)and(cx<=w)and(cy<=h)
end


function Object:CallChildrenHT(eventname, x, y, ...)
  local children = self.children
  for i=1,#children do
    local c = children[i]
    if (c) then
      local cx,cy = c:ParentToLocal(x,y)
      if InLocalRect(cx,cy,c.width,c.height) and c:HitTest(cx,cy) then
        local obj = c[eventname](c, cx, cy, ...)
        if (obj) then
          return obj
        end
      end
    end
  end
end


function Object:InLocalRect(x, y)
  return (x >= 0) and (y >= 0) and (x <= self.width) and (y <= self.height)
end

function Object:CallChildrenHTWeak(eventname, x, y, ...)
  local children = self.children
  for i=1,#children do
    local c = children[i]
    if (c) then
      local cx,cy = c:ParentToLocal(x,y)
      if InLocalRect(cx,cy,c.width,c.height) then
        local obj = c[eventname](c, cx, cy, ...)
        if (obj) then
          return obj
        end
      end
    end
  end
end

--//=============================================================================

function Object:RequestUpdate()
  --// we have something todo in Update
  --// so we register this object in the taskhandler
  TaskHandler.AddObject(self)
end


function Object:Invalidate()
  --FIXME should be Control only
end


function Object:Draw()
  self:CallChildrenInverse('Draw')
end


function Object:TweakDraw()
  self:CallChildrenInverse('TweakDraw')
end

--//=============================================================================

function Object:LocalToParent(x,y)
  return x + self.x, y + self.y
end


function Object:ParentToLocal(x,y)
  return x - self.x, y - self.y
end


Object.ParentToClient = Object.ParentToLocal
Object.ClientToParent = Object.LocalToParent


function Object:LocalToClient(x,y)
  return x,y
end


function Object:LocalToScreen(x,y)
  if (not self.parent) then
    return x,y
  end
  --Spring.Echo((not self.parent) and debug.traceback())
  return (self.parent):ClientToScreen(self:LocalToParent(x,y))
end


function Object:ClientToScreen(x,y)
  if (not self.parent) then
    return self:ClientToParent(x,y)
  end
  return (self.parent):ClientToScreen(self:ClientToParent(x,y))
end


function Object:ScreenToLocal(x,y)
  if (not self.parent) then
    return self:ParentToLocal(x,y)
  end
  return self:ParentToLocal((self.parent):ScreenToClient(x,y))
end


function Object:ScreenToClient(x,y)
  return self:ParentToClient((self.parent):ScreenToClient(x,y))
end

--//=============================================================================

function Object:_GetMaxChildConstraints(child)
  return 0, 0, self.width, self.height
end

--//=============================================================================


function Object:HitTest(x,y)
  if not self.disableChildrenHitTest then 
    local children = self.children
    for i=1,#children do
      local c = children[i]
      if (c) then
        local cx,cy = c:ParentToLocal(x,y)
        if InLocalRect(cx,cy,c.width,c.height) then
          local obj = c:HitTest(cx,cy)
          if (obj) then
            return obj
          end
        end
      end
    end
  end 

  return false
end


function Object:IsAbove(x, y, ...)
  return self:HitTest(x,y)
end


function Object:MouseMove(...)
  if (self:CallListeners(self.OnMouseMove, ...)) then
    return self
  end

  return self:CallChildrenHT('MouseMove', ...)
end


function Object:MouseDown(...)
  if (self:CallListeners(self.OnMouseDown, ...)) then
    return self
  end

  return self:CallChildrenHT('MouseDown', ...)
end


function Object:MouseUp(...)
  if (self:CallListeners(self.OnMouseUp, ...)) then
    return self
  end

  return self:CallChildrenHT('MouseUp', ...)
end


function Object:MouseClick(...)
  if (self:CallListeners(self.OnClick, ...)) then
    return self
  end

  return self:CallChildrenHT('MouseClick', ...)
end


function Object:MouseDblClick(...)
  if (self:CallListeners(self.OnDblClick, ...)) then
    return self
  end

  return self:CallChildrenHT('MouseDblClick', ...)
end


function Object:MouseWheel(...)
  if (self:CallListeners(self.OnMouseWheel, ...)) then
    return self
  end

  return self:CallChildrenHTWeak('MouseWheel', ...)
end

-- cursor stuff

function Object:AddCursor(...)
  if (self:CallListeners(self.OnAddCursor, ...)) then
    return self
  end

  return self:CallChildrenHT('AddCursor', ...)
end

function Object:UpdateCursor(...)
  if (self:CallListeners(self.OnUpdateCursor, ...)) then
    return self
  end

  return self:CallChildrenHT('UpdateCursor', ...)
end

function Object:CursorEntered(...)
  
  if (self:CallListeners(self.OnCursorEntered, ...)) then
    return self
  end

  return self:CallChildrenHT('CursorEntered', ...)
end

function Object:RemoveCursor(...)
  if (self:CallListeners(self.OnRemoveCursor, ...)) then
    return self
  end

  return self:CallChildrenHT('RemoveCursor', ...)
end

function Object:CursorExited(...)
  if (self:CallListeners(self.OnCursorExited, ...)) then
    return self
  end

  return self:CallChildrenHT('CursorExited', ...)
end

function Object:TouchTap(...)
  if (self:CallListeners(self.OnTouchTap, ...)) then
    return self
  end

  return self:CallChildrenHT('TouchTap', ...)
end

function Object:RefreshCursors(...)
  if (self:CallListeners(self.OnRefreshCursors, ...)) then
    return self
  end

  self:CallChildren('RefreshCursors', ...)
  return false
end

--//=============================================================================





