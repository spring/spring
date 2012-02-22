--//=============================================================================
--//  SHORT INFO WHY WE DO THIS:
--// Cause of the reference based system in lua we can't 
--// destroy objects yourself, instead we have to tell
--// the GarbageCollector somehow that an object isn't
--// in use anymore.
--//  Now we have a quite complex class system in Chili 
--// with parent and children links between objects. Those
--// circles make it normally impossible for the GC to
--// detect if an object (and all its children) can be
--// destructed.
--//  This is the point where so called WeakLinks come
--// into play. Instead of saving direct references to the
--// objects in the parent field, we created a weaktable
--// (use google, if you aren't familiar with this name)
--// which directs to the parent-object. So now the link
--// between children and its parent is 'weak' (the GC can
--// catch the parent), and the link between the parent
--// and its children is 'hard', so the GC won't catch the
--// children as long as there is a parent object.
--//=============================================================================

local wmeta =  {__mode = "v"}

function MakeWeakLink(obj,wlink)
  --// 2nd argument is optional, if it's given it will reuse the given link (-> less garbage)

  obj = UnlinkSafe(obj) --// unlink hard-/weak-links -> faster (else it would have to go through multiple metatables)

  if (not isindexable(obj)) then
    return obj
  end

  local mtab
  if (type(wlink) == "userdata") then
    mtab = getmetatable(wlink)
  end
  if (not mtab) then
    wlink = newproxy(true)
    mtab = getmetatable(wlink)
    setmetatable(mtab, wmeta)
  end
  local getRealObject = function() return mtab._obj end --// note: we are using mtab._obj here, so it is a weaklink -> it can return nil!
  mtab._islink = true
  mtab._isweak = true
  mtab._obj = obj
  mtab.__index = obj
  mtab.__newindex = obj
  mtab.__call = getRealObject --// values are weak, so we need to make it gc-safe
  mtab[getRealObject] = true  --// and buffer it in a key, too

  if (not obj._wlinks) then
    local t = {}
    setmetatable(t,{__mode="v"})
    obj._wlinks = t
  end
  obj._wlinks[#obj._wlinks+1] = wlink

  return wlink
end


function MakeHardLink(obj,gc)
  if (not isindexable(obj)) then
    return obj
  end

  obj = UnlinkSafe(obj) --// unlink hard-/weak-links -> faster (else it would have to go through multiple metatables)

  local hlink = newproxy(true)
  local mtab = getmetatable(hlink)
  mtab._islink = true
  mtab._ishard = true
  mtab._obj = obj
  mtab.__gc = gc
  mtab.__index = obj
  mtab.__newindex = obj
  mtab.__call = function() return mtab._obj end

  if (not obj._hlinks) then
    local t = {}
    setmetatable(t,{__mode="v"})
    obj._hlinks = t
  end
  obj._hlinks[#obj._hlinks+1] = hlink

  return hlink
end


function Unlink(link)
  --// this doesn't do a type check!
  --// you have to be 100% sure it is a link!
  return link and link()
end


function UnlinkSafe(link)
  return ((type(link) == "userdata") and link()) or link
end


function CompareLinks(link1,link2)
  return UnlinkSafe(link1) == UnlinkSafe(link2)
end


function CheckWeakLink(link)
  return (type(link) == "userdata") and link()
end

--//=============================================================================
