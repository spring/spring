--//=============================================================================
--// TaskHandler

TaskHandler = {}

--// use use 2 tables so we can iterate and update at the same time
local objects = {}
local objects2 = {}
local objectsCount = 0

--// make it a weak table (values)
do
  local m = {__mode = "v"}
  setmetatable(objects, m)
  setmetatable(objects2, m)
end

--//=============================================================================

--// Global Event for when an object gets destructed
local globalDisposeListeners = {}
setmetatable(globalDisposeListeners, {__mode = "k"})

local function CallListenersByKey(listeners, ...)
  for obj in pairs(listeners) do
    obj:OnGlobalDispose(...);
  end
end


function TaskHandler.RequestGlobalDispose(obj)
  globalDisposeListeners[obj] = true
end

--//=============================================================================

function TaskHandler.AddObject(obj)
  obj = UnlinkSafe(obj)
  if (not obj.__inUpdateQueue) then
    obj.__inUpdateQueue = true
    objectsCount = objectsCount + 1
    objects[objectsCount] = obj
  end
end


function TaskHandler.RemoveObject(obj)
  obj = UnlinkSafe(obj)

  CallListenersByKey(globalDisposeListeners, obj)

  if (obj.__inUpdateQueue) then
    obj.__inUpdateQueue = false
    for i=1,objectsCount do
      if (objects[i]==obj) then
        objects[i] = objects[objectsCount]
        objects[objectsCount] = nil
        objectsCount = objectsCount - 1
        return true
      end
    end
    return false
  end
end

--//=============================================================================

function TaskHandler.Update()
  --if (debug) then
  --  collectgarbage("collect")
  --end

  local obj
  local Update
  local cnt = objectsCount
  objectsCount = 0 --// clear the array, so all objects needs to reinsert themselves when they want to get called again
  objects,objects2 = objects2,objects
  for i=1,cnt do
    obj = objects2[i]
    if (obj)and(not obj.disposed) then
      obj.__inUpdateQueue = false
      Update = obj.Update
      if (Update) then
        Update(obj)
      end
    end
  end

end

--//=============================================================================
