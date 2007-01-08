--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    savetable.lua
--  brief:   a human friendly table writer
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

if (table.save) then
  return
end

local indentString = '\t'

local savedTables = {}

local function encloseStr(s)
  return string.format('%q', s)
end

local function encloseKey(s)
  local wrap = string.find(s, '[^%w]')
  if (not wrap) then
    if (string.len(s) <= 0) then wrap = true end
  end
  if (not wrap) then
    local keywords = {
      ['and'] = true, ['break'] = true, ['do'] = true, ['else'] = true,
      ['elseif'] = true, ['end'] = true, ['false'] = true, ['for'] = true,
      ['function'] = true, ['if'] = true, ['in'] = true, ['local'] = true,
      ['nil'] = true, ['not'] = true, ['or'] = true, ['repeat'] = true,
      ['return'] = true, ['then'] = true, ['true'] = true, ['until'] = true,
      ['while'] = true
    }
    if (keywords[s]) then wrap = true end
  end
    
  if (wrap) then
    return string.format('[%q]', s)
  else
    return s
  end
end


local keyTypes = {
  ['string']  = true,
  ['number']  = true,
  ['boolean'] = true
}

local valueTypes = {
  ['string']  = true,
  ['number']  = true,
  ['boolean'] = true,
  ['table']   = true
}


local function CompareKeys(kv1, kv2)
  local k1, v1 = kv1[1], kv1[2]
  local k2, v2 = kv2[1], kv2[2]
  if (ktype1 ~= ktype2) then
    return (ktype1 < ktype2)
  end
---[[
  local vtype1 = type(v1)
  local vtype2 = type(v2)
  if ((vtype1 == 'table') and (vtype2 ~= 'table')) then
    return false
  end
  if ((vtype1 ~= 'table') and (vtype2 == 'table')) then
    return true
  end
--]]
  return (k1 < k2)
end


local function MakeSortedTable(t)
  local st = {}
  for k,v in pairs(t) do
    if (keyTypes[type(k)] and valueTypes[type(v)]) then
      table.insert(st, { k, v })
    end
  end
  table.sort(st, CompareKeys)
  return st
end


local function SaveTable(t, file, indent)
  file:write('{\n')
  local indent = indent .. indentString
  
  local st = MakeSortedTable(t)
  
  for _,kv in ipairs(st) do
    local k, v = kv[1], kv[2]
    local ktype = type(k)
    local vtype = type(v)
    -- output the key
    if (ktype == 'string') then
      file:write(indent..encloseKey(k)..' = ')
    else
      file:write(indent..'['..tostring(k)..'] = ')
    end
    -- output the value
    if (vtype == 'string') then
      file:write(encloseStr(v)..',\n')
    elseif (vtype == 'number') then
      file:write(tostring(v)..',\n')
    elseif (vtype == 'boolean') then
      file:write(tostring(v)..',\n')
    elseif (vtype == 'table') then
      if (savedTables[v]) then
        error("table.save() does not support recursive tables")
      end
      if (next(v)) then
        savedTables[t] = true
        SaveTable(v, file, indent)
        file:write(indent..'},\n')
        savedTables[t] = nil
      else
        file:write('{},\n') -- empty table
      end
    end
  end
end


function table.save(t, filename, header)
  local file = io.open(filename, 'w')
  if (header) then
    file:write(header..'\n')
  end
  file:write('return ')
  SaveTable(t, file, '')
  file:write('}\n')
  file:close()
  for k,v in pairs(savedTables) do
    savedTables[k] = nil
  end
end
