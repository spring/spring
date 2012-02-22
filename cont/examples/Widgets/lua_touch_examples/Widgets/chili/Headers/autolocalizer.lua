---------------------------------------------------------------------------
--
-- Lua Namespace Localizer
--
-- author: jK (2009)
--
-- License: GPLv2
--
-- Info:
-- overrides VFS.Include with a function that autolocalize the used namespaces
--

-- Settings
local maxLocals = 50

local namespaces = {
  {"Spring.MoveCtrl", "mc"},
  {"Spring", "sp"},
  {"Script", "sc"},
  {"gl", "gl"},
  {"GL", "GL_"},
  {"math", ""},
  {"CMD", "CMD_"},
  {"CMDTYPE", "CMDTYPE_"},
  {"Game", "Game"},
  {"table", "tb"},
}


--//TODO
local globalFuncs = {
  "pairs",
  "ipairs",
  "select",
  "unpack",
  "UnitDefs",
  "UnitDefNames",
  "FeatureDefs",
  "WeaponDefs",
}

---------------------------------------------------------------------------
--

if (Spring.GetConfigInt("AutoLocalizeLua", 1) == 0) then
  return
end

---------------------------------------------------------------------------
--

local used_funcs = {}
local numLocals = 0

local function CreateLocal(prefix)
  local _G = getfenv()
  return function(fnc_prefix,fnc_name)
    local longname  = fnc_prefix .. "." .. fnc_name
    local shortname = prefix..fnc_name

    --// if it's not in the current global namespace (perhaps it gets added later?)
    --// then don't localize it
    local gn = _G[fnc_prefix]
    if (type(gn)~="table")or(not gn[fnc_name]) then
      return longname
    end

    if (used_funcs[longname]) then
      return shortname
    else
      if (numLocals<maxLocals) then
        numLocals = numLocals + 1
        used_funcs[longname] = prefix..fnc_name
        return shortname
      else
        return longname --// don't localize we hit locals limit
      end
    end
  end
end


local function LocalizeString(str)
  used_funcs = {}
  numLocals = 0

  for i=1, #namespaces do
    local ns = namespaces[i]
    str = str:gsub( "(".. ns[1] .. ")%.([%w_]+)", CreateLocal(ns[2]))
  end

  for i=1, #globalFuncs do
    local gfunc = globalFuncs[i]
    if (str:find( "[^%w_]" .. gfunc .. "[^%w_]" )) then
      used_funcs[gfunc] = gfunc
    end
  end

  local array = {}
  local i = 1
  for funcname, shortname in pairs(used_funcs) do
    array[i] = "local " .. shortname .. " = " .. funcname ..";"
    i = i + 1
  end
  array[i] = str

  return table.concat(array, "")
end

---------------------------------------------------------------------------
--

local orig_load = VFS.LoadFile
local orig_include = VFS.Include

function LoadFileSafe(filename, mode)
  if (mode) then
    return orig_load(filename, mode)
  else
    return orig_load(filename)
  end
end

---------------------------------------------------------------------------
--

local function MyLoadString(str, filename)
  local chunk,err
  local oldMaxLocals = maxLocals
  local str2

  if (filename:len()>40) then
    filename = filename:sub(-40)
  end

  repeat
    str2 = LocalizeString(str)

    chunk, err = loadstring(str2, filename)

    if (not chunk) then
      if (err:find("has more than %d+ upvalues")) then
        maxLocals = maxLocals - 10
      end
      error(err)
    end

  until (chunk)or(maxLocals==0)

  maxLocals = oldMaxLocals

  return chunk,str2
end


function VFS.Include(filename, enviroment, mode)
  local str = LoadFileSafe(filename, mode)

  if (not str) then
    return
  end

  if (not enviroment) then
    local status
    status,enviroment = pcall(getfenv,3)
    if (not status) then
      status,enviroment = pcall(getfenv,2)
      if (not status) then
        enviroment = getfenv()
      end
    end
  end

  local chunk = MyLoadString(str, filename)
  setfenv(chunk, enviroment)

  return chunk()
end


function VFS.LoadFile(filename, mode)
  local str = LoadFileSafe(filename, mode)

  if (str)and(filename:find(".lua", 1, true)) then
    _,str = MyLoadString(str, filename)
  end

  return str
end