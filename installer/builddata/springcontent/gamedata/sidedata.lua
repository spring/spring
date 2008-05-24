--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    sidedata.lua
--  brief:   sidedata.tdf lua parser
--  author:  Craig Lawrence, Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local TDF = VFS.Include('gamedata/parse_tdf.lua')

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

if (not VFS.FileExists('gamedata/sidedata.tdf')) then
  return false
end

local sideData, err = TDF.Parse('gamedata/sidedata.tdf')
if (sideData == nil) then
  error('Error parsing sidedata.tdf: ' .. err)
end

local luaSides = {}
local index = 0
while (true) do
  local label = 'side' .. index -- tdf names start at 'side0'
  local data = sideData[label]
  if (type(data) ~= 'table') then
    break
  else
    print(string.format('%s "%s" "%s"',
                        label, tostring(data.name), tostring(data.commander)))
    -- rename 'commander' to 'startunit'
    data['startunit'] = data['commander']
    data['commander'] = nil
    index = index + 1
    luaSides[index] = data -- lua indices start at 1
  end
end


--------------------------------------------------------------------------------

return luaSides

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
