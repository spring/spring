#!/usr/bin/env lua

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    lua_localizer.lua
--  brief:   localizes some lua table entries for Spring widgets / gadgets
--  author:  Dave Rodgers  (aka: trepan)
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

require('io')
require('string')
require('table')

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local subTable = {
  --              mixed,      all caps
--  ['Script']  = { 'sc',       'SC_'      }, --  na, don't do this one
  ['Spring']  = { 'sp',       'SP_'      },
  ['gl']      = { 'gl',       'GL_'      },
  ['GL']      = { 'GL',       'GL_'      },
  ['CMD']     = { 'CMD_',     'CMD_'     },
  ['CMDTYPE'] = { 'CMDTYPE_', 'CMDTYPE_' },
}

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local execName = arg[0]
local fileName = arg[1]


if (type(fileName) ~= 'string') then
  print('Usage: ' .. execName .. ' <filename>')
end


local f, err = io.open(fileName, 'r')
if (f == nil) then
  print(err)
  return
end

local text = f:read('*a')


local function IsAllCaps(text)
  return (string.find(text, "%l") == nil)
end


local localSubs = {}


for key, subs in pairs(subTable) do
  local lower = subs[1]
  local upper = subs[2]

  local function Substitute(text, extra)
    if (not extra) then
      extra = ''
    end
    if (extra == '.') then
      return text .. extra  -- no substitution, nested tables
    end
    local call = string.sub(text, #key + 2)
    local prefix = IsAllCaps(call) and upper or lower
    local subText = prefix .. call

    localSubs[subText] = text

    return subText .. extra
  end

  text = string.gsub(text, '(' .. key .. '%.%a[%a%d_]*)(.)', Substitute)
end
  

-- sort the entries for printing, and find the max string length
local maxLen = 0
local sorted = {}
for lText, oText in pairs(localSubs) do
  table.insert(sorted, { lText, oText })
  maxLen = (maxLen > #lText) and maxLen or #lText
end
table.sort(sorted, function(a,b) return a[1] < b[1] end)


-- prepend the local definitions
local newText = ''

newText = newText .. string.rep('-', 80) .. '\n'
newText = newText .. string.rep('-', 80) .. '\n'
newText = newText .. '\n'
newText = newText .. '-- Automatically generated local definitions\n'
newText = newText .. '\n'

for _, newOld in ipairs(sorted) do
  local fmt = 'local %-' .. maxLen .. 's = %s\n'
  local def = string.format(fmt, newOld[1], newOld[2])
  newText = newText .. def
  print(string.sub(def, 1, -2))
end

newText = newText .. '\n'
newText = newText .. string.rep('-', 80) .. '\n'
newText = newText .. string.rep('-', 80) .. '\n'
newText = newText .. '\n'

newText = newText .. text


-- output the modified text
local outName = string.gsub(fileName, '.lua$', '.locals.lua')
local out, err = io.open(outName, 'w')
if (out == nil) then
  print(err)
else
  out:write(newText)
  out:close()
  print('\nSaved ' .. outName .. '\n')
end


f:close()


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
