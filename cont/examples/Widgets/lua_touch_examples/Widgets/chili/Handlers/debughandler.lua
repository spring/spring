--//=============================================================================
--// Chili DebugHandler
--//=============================================================================

--// todo:
--// * use the custom errorhandler for any widget->chili interaction,
--//   atm if a widget calls mybutton:SetCaption() the widgethandler errorhandler is used,
--//   but this one uses pcall and so it can't generate a stacktrace, which makes debugging
--//   nearly impossible (cuz the error mostly occurs in a ver low level chili object as `Font`).
--//   So any :New{} should in thoery return a wrapped table to the widgets, I already tried
--//   to wrap the whole WG.Chili namespace (check the `SafeWrap` function beneath), but my tests
--//   ended in crashes in chili, cuz such wrapped tables ended in chili itself
--//   (chili should only use links and direct ones, but never wrapped tables)


DebugHandler = {}

--//=============================================================================
--// List of all chili object for easy debugging

--// DebugHandler.objectsOwnedByWidgets[widget][n] = owned_object
--// hint: because the per-widget table is a weaktable you still need to use pairs() to iterate it!
DebugHandler.objectsOwnedByWidgets = {}
setmetatable(DebugHandler.objectsOwnedByWidgets, {
  __mode="k",
  __index=function(t,i)
    local st = {};
    setmetatable(st,{__mode="v"});
    t[i] = st;
    return st;
  end,
})

--// holds all created chili objects (uses a weaktable to do so)
--// hint: because it is a weaktable you still need to use pairs() to iterate it!
DebugHandler.allObjects = {}
setmetatable(DebugHandler.allObjects, {
  __mode="v",
})


--//=============================================================================
--// Chili ErrorHandler 
--//
--// Chili is a framework, so many widgets move parts of their code into its
--// thread. If now a widget error happens, it could be shown as chili errors and
--// the widgethandler would unload chili instead of the causing widget.
--//
--// Now this errorhandler removes the widget itself (and its owned chili objects)
--// w/o crashing the whole framework.

DebugHandler.maxStackLength = 7
DebugHandler.maxChiliErrors = 5
local numChiliErrors = 0
local lastError = 0

local function ChiliErrorHandler(msg,...)
  local control 
  local _i = 2 --// 1 is this function, so skip it
  repeat
    if (not debug.getinfo(_i)) then
      break
    end

    local i = 1
    repeat
      local name,value = debug.getlocal(_i, i)
      if (not name) then
        break
      end
      if (name == "self") then
        if (true) then
          control = value
        end
      end

      i = i + 1
    until(control)

    _i = _i + 1
  until(control)

  if (control) then
    local w = control._widget
    if (w) then
      Spring.Echo(("Chili-Error in `%s`:%s : %s"):format(w.whInfo.name, control.name, msg))
      Spring.Echo(DebugHandler.Stacktrace())

      --// this also unloads all owned chili objects
      Spring.Echo("Removed widget: " .. w.whInfo.name)
      widgetHandler:RemoveWidget(w)
    else
      Spring.Echo(("Chili-Error in `%s` (couldn't detect the owner widget): %s"):format(control.name, msg))
      Spring.Echo(DebugHandler.Stacktrace())
      control:Dispose()
    end
  else
    --// error in Chili itself
    Spring.Echo(("Chili-Error: %s"):format(msg))
    Spring.Echo(DebugHandler.Stacktrace())

    if (os.clock() - lastError < 5) then
      numChiliErrors = numChiliErrors + 1
    else
      numChiliErrors = 1
    end
    lastError = os.clock()

    --// unload Chili to avoid error spam
    if (numChiliErrors>=DebugHandler.maxChiliErrors) then
      Spring.Echo("Removed widget: " .. widget.whInfo.name)
      widgetHandler:RemoveWidget(widget)
    end
  end
end
DebugHandler.ChiliErrorHandler = ChiliErrorHandler


--//=============================================================================
--// Stacktrace

function DebugHandler.Stacktrace()
  local self_src = debug.getinfo(1).short_src

  local trace = {"stacktrace:"}
  local _i = 3
  repeat
    local info = debug.getinfo(_i)
    if (not info) then break end

    --// we remove any occurence of the debughandler in the stacktrace (it's useless for debugging)
    if (info.short_src ~= self_src)
       and(info.name ~= "SafeCall")
       and(info.name ~= "cdxpcall")
       and(info.name ~= "cdCreateList")
    then
      local curLine = info.currentline
      if (curLine<0) then
        curLine = ""
      else
        curLine = ":" .. curLine
      end
      if (info.name) then
        if (info.name=="") then
          trace[#trace+1] = ("%s%s: in [?]"):format(info.short_src,curLine)
        else
          trace[#trace+1] = ("%s%s: in %s"):format(info.short_src,curLine,info.name)
        end
      else
        trace[#trace+1] = ("%s%s"):format(info.short_src,curLine)
      end
    end

    _i = _i + 1
  until(false)

  if ((#trace-1) > DebugHandler.maxStackLength) then
    local i = 1 + math.ceil(DebugHandler.maxStackLength/2)+1
    local tail = (DebugHandler.maxStackLength - i)
    local j = #trace - tail
    trace[i] = "..."
    for n=1,tail do
      trace[i+n] = trace[j+n]
    end
    for i=1+DebugHandler.maxStackLength+1, #trace do
      trace[i] = nil
    end
  end

  return table.concat(trace,"\n\t")
end


--//=============================================================================
--// Define some helper functions needed for xpcall

--// we do this so we can identify it in Stacktrace() (cd = chili debug)
local cdxpcall = xpcall

local sharedEnv = {_f = nil}
local setArgsFuncs = {}
local callFuncs = {}
do
  local function CreateNewHelpers(_,n)
    local t0 = {}
    local t1 = {}
    local t2 = {}
    for i=1,n do
      t0[i] = "a" .. i
      t1[i] = "arg" .. i
      t2[i] = "a" .. i .. "=arg" .. i
    end
    local varargs = table.concat(t0, ", ")
    local arglist = table.concat(t1, ",")
    local setargs = table.concat(t2, "; ")

    local src = ([[local %s;
	local setargs = function(%s) %s; end;
	local call = function() return _f(%s); end
        return setargs,call
    ]]):format(varargs, arglist, setargs, varargs)

    local setargsfunc,callfunc = assert(loadstring(src))()
    setfenv(setargsfunc,sharedEnv)
    setfenv(callfunc,sharedEnv)
    rawset(setArgsFuncs, n, setargsfunc)
    rawset(callFuncs, n, callfunc)
    return setargsfunc
  end
  setmetatable(setArgsFuncs, {__index = CreateNewHelpers})
  setmetatable(callFuncs, {__index = CreateNewHelpers})
end


local function GetResults(status,...)
  if (status) then
    return ...
  else
    return nil
  end
end


local function xpcall_va(func, ...)
  local n = select("#", ...)
  if (n == 0) then
    return GetResults(cdxpcall(func,ChiliErrorHandler))
  else
    sharedEnv._f = func
    setArgsFuncs[n](...)
    return GetResults(cdxpcall(callFuncs[n],ChiliErrorHandler))
  end
end


--//=============================================================================
--// Define Chili.SafeCall

DebugHandler.SafeCall = xpcall_va
SafeCall = xpcall_va --// external code should access it via Chili.SafeCall()


--//=============================================================================
--// Use our errorhandler in gl.CreateList

local orig_gl = gl
local cdCreateList = gl.CreateList
gl = {}
setmetatable(gl,{__index=function(t,i) t[i] = orig_gl[i]; return t[i]; end}) --// use metatable to copy table entries on-demand
gl.CreateList = function(...)
	return cdCreateList(xpcall_va, ...)
end



--//=============================================================================
--// Chili.DebugHandler.SafeWrap
--// usage:
--//   used for WG.Chili & screen0 to make any calling to them safe
--// how:
--//   it wraps a table, so any calling to a function in it (or ones of its subtables)
--//   uses automatically the Chili internal error handler
--// note:
--//   it doesn't auto wrap th return values of such functions (for performance reasons)


local function IndexWrappedTable(t,i)
  local v_orig = t.__orig[i]

  local v
  if (isindexable(v_orig)) then
    --Spring.Echo("wrap table", i)
    v = DebugHandler.SafeWrap(v_orig);
    t[i] = v;
  elseif (isfunc(v_orig)) then
    --Spring.Echo("wrap func", i)
    v = function(self,...)
      if (isindexable(self))and(self.__orig) then
        self = self.__orig
      end
      return DebugHandler.SafeWrap(xpcall_va(v_orig,self,...));
    end;
    t[i] = v;
  else
    v = v_orig;
    --t[i] = v;  don't cache values (else they wouldn't update when they got in the original table)
  end
  return v;
end

local function HasSubFunctions(t)
  if (not istable(t)) then
    return true
  end
  for i,v in pairs(t) do
    if (isfunc(v)) then
      return true
    elseif (isindexable(v)) then
      local r = HasSubFunctions(v)
      if (r) then return true end
    end
  end
  return false
end

local function IndexWrappedLink(link,i)
  local t = link()
  local v_orig = t[i]
  --local t_wrap = getmetatable(link)._wrap

  local v = v_orig;
  if (isindexable(v_orig)) then
  --  Spring.Echo("wrap table", i,HasSubFunctions(v_orig))
  --  if (HasSubFunctions(v_orig)) then
  --    v = DebugHandler.SafeWrap(v_orig);
  --    t_wrap[i] = v;
  --  end
  elseif (isfunc(v_orig)) then
    --Spring.Echo("wrap func", i)
    v = function(self,...)
      if (isindexable(self))and(self.__orig) then
        self = self.__orig
      end
      return xpcall_va(v_orig,self,...);
      --return DebugHandler.SafeWrap(xpcall_va(v_orig,self,...));
    end;
    --t_wrap[i] = v;
  end
  return v;
end

local safewrap_meta = {
  __index = IndexWrappedTable
}

function DebugHandler.SafeWrap(t_orig,...)
  if isindexable(t_orig) then
    local meta = getmetatable(t_orig)
    if (meta)and(meta._islink) then
      meta._wrap = {}
      meta[meta._wrap] = true
      meta.__index = IndexWrappedLink
      return t_orig
    else
      local t = {__orig = t_orig}
      setmetatable(t, safewrap_meta)
      return t
    end
  else
    return t_orig,...
  end
end


--//=============================================================================
--// When a Widget gets removed destroy all its owned chili object, too

local origRemoveWidget = widgetHandler.RemoveWidget
widgetHandler.RemoveWidget = function(self,widget,...)
  local ownedObjects = DebugHandler.objectsOwnedByWidgets[widget]
  for k,obj in pairs(ownedObjects) do
    obj:Dispose()
    ownedObjects[k] = nil
  end

  origRemoveWidget(self,widget,...)
end


--//=============================================================================
--// Check the stack for any indication for an user widget
--//  so we can remove all owned chili object when the widget crashes/unloads
--//  (also useful for a lot of other debugging purposes)

function DebugHandler.GetWidgetOrigin(_i)
  local chiliwidget = widget
  _i = _i or 4 --// 1:=this function, 2:=RegisterObject, 3:=class:New{}, 4:=user widget?
  repeat
    --// CHECK STACK
    local info = debug.getinfo(_i,"f")
    if (not info) then
      return
    end

    --// CHECK ENVIROMENT
    if (info.func) then
      local _G = debug.getfenv(info.func)
      if (_G.widget)and(_G.widget~=chiliwidget)and(_G.widget.GetInfo) then
        --Spring.Echo("env widget found", _G.widget:GetInfo().name)
        return _G.widget
      end
    end

    --// CHECK LOCALS
    local i=1
    repeat
      local name,value = debug.getlocal(_i, i)
      if (not name) then
        break
      end
      if (name == "widget")and(value.GetInfo) then
        --Spring.Echo("local widget found", value:GetInfo().name)
        return value
      else
        --// this code gets triggered when a chili control creates another one
        --// (the new control should then inherit the widget origin)
        local typ = type(value)
        if (typ=="table")or(typ=="metatable")or(typ=="userdata") then
          if (typ._widget)and(typ._widget.GetInfo) then
            --Spring.Echo("local2 widget found", typ._widget:GetInfo().name)
            return typ._widget
          end
        end
      end
      i = i + 1
    until(false)

    _i = _i + 1
  until(false)
end


--//=============================================================================
--// gets called by Chili.Object:New{}

function DebugHandler:RegisterObject(obj)
  local w = self.GetWidgetOrigin()
  if (w) then
    obj._widget = w
    local t = self.objectsOwnedByWidgets[w]
    t[#t+1] = obj
  end
  local ta = self.allObjects
  ta[#ta+1] = obj
  --// hint: both tables above are weaktables, so we don't need a UnregisterObject!
end

--//=============================================================================
