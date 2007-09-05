#!/usr/bin/env lua
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    param_types.lua
--  brief:   utility to help pick-out LuaParser param types from C++ source
--  author:  Dave Rodgers
--
--  notes:
--    example usage:  ./param_types UnitDefHandler.cpp
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

require 'io'
require 'string'

local srcFile = arg[1]
if (srcFile == nil) then
  error('usage:  '..arg[0]..' <srcFile>')
end

local types = {
  'Int',
  'Bool',
  'Float',
  'Float3',
  'String',
}

local maps = {}


local f, err = io.open(srcFile, 'r')
if (f == nil) then
  error(err)
end


local text = f:read('*a')


local function collect_names(type)
  local map = {}
  local array = {}
  f:seek('set')
  for text in f:lines() do
    for caller, result in string.gfind(text, '([%a%d_]*).Get'..type..'%("([^"]*)"') do
      table.insert(array, { result, caller, text })
      map[result] = true
    end
  end
  table.sort(array, function(a, b) return a[1] < b[1] end)
  maps[type] = map
  return array
end


local function print_names(type, array)
  print('local '..string.lower(type)..'Map = {')
  for i, d in ipairs(array) do
    local result = d[1]
    local caller = d[2]
    local line   = string.gsub(d[3], '^%s*', '')
    print(string.format('  %-24s = true, -- %s -- %s --',
                        result, caller, result))
  end
  print('}')
end


local function parse_type(type)
  local array = collect_names(type)
  print_names(type, array)
end

for _, type in ipairs(types) do
  parse_type(type)
  print()
end

print('return {')
for _, type in ipairs(types) do
  local name = string.lower(type)..'Map'
  print(string.format('  %-9s = %s,', name, name))
end
print('}')
print()


local function collect_subtables()
  local array = {}
  f:seek('set')
  for text in f:lines() do
    local s, e = string.find(text, 'SubTable')
    if (s) then
      print('-- SubTable: '..text)
    end
  end
end


collect_subtables()


for _, type1 in ipairs(types) do
  for _, type2 in ipairs(types) do
    if (type1 ~= type2) then
      for name in pairs(maps[type1]) do
--        print(type1, type2, name)
        if (type2[name] ~= nil) then
          print(string.format(
            'error("Type conflict: %s is both a %s and %s")',
          name, type1, type2))
        end
      end
    end
  end
end

f:close()
