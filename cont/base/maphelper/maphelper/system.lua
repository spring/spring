--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    system.lua
--  brief:   system calls table
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Utility that traverses a table tree and converts string keys to lowercase
--
--  NOTE:  metatable are not affected  (pairs() does not use them)
--

local lowerkeys

do
  local lowerMap = {}

  local function lowerkeys2(t)
    if (lowerMap[t]) then
      return  -- avoid recursion / repetition
    end

    lowerMap[t] = true

    local changes = {}
    for k, v in pairs(t) do
      if (type(k) == 'string') then
        local l = string.lower(k)
        if (l ~= k) then
          if (t[l] == nil) then
            changes[l] = v
          end
          t[k] = nil
        end
      end
      if (type(v) == 'table') then
        lowerkeys2(v)
      end
    end

    -- insert new keys outside of the pairs() loop
    for k, v in pairs(changes) do
      t[k] = v
    end
  end

  lowerkeys = function(t)
    lowerMap = {}
    lowerkeys2(t)
    return t  -- convenience, do not mistake this for a copy
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

-- no metatable protection
local function reftable(ref, tbl)
  tbl = tbl or {}
  setmetatable(tbl, { __index = ref })
  return tbl
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local system = {

  --  Custom packages
  VFS = VFS,
  Spring = Spring,

  --  Custom functions
  lowerkeys = lowerkeys,
  reftable = reftable,

  --  Custom tables
  Map = Map,

  --  Standard packages
  math = math,
  table = table,
  string = string,
  coroutine = coroutine,
  
  --  
  --  Standard functions and variables
  --
  assert         = assert,
  error          = error,

  print          = print,
  
  next           = next,
  pairs          = pairs,
  ipairs         = ipairs,

  tonumber       = tonumber,
  tostring       = tostring,
  type           = type,

  --collectgarbage = collectgarbage,
  --gcinfo         = gcinfo,

  unpack         = unpack,
  select         = select,

  --dofile         = dofile,
  --loadfile       = loadfile,
  --loadlib        = loadlib,
  loadstring     = loadstring,
  --require        = require,

  getmetatable   = getmetatable,
  setmetatable   = setmetatable,

  rawequal       = rawequal,
  rawget         = rawget,
  rawset         = rawset,

  getfenv        = getfenv,
  setfenv        = setfenv,

  pcall          = pcall,
  xpcall         = xpcall,

  _VERSION       = _VERSION
}


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

setmetatable(system, {
  __newindex  = function() error('Attempt to write to system') end,
  __metatable = function() error('Attempt to access system metatable') end,
})


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

return system

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
