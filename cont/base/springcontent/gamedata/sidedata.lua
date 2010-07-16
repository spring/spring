--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:     sidedata.lua
--  brief:    sidedata.tdf lua parser
--  authors:  Dave Rodgers, Craig Lawrence
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------


local tdfFile = 'gamedata/sidedata.tdf'
if (not VFS.FileExists(tdfFile)) then
  return {}
end


local TDF = VFS.Include('gamedata/parse_tdf.lua')
local sideData, err = TDF.Parse(tdfFile)
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
    -- rename 'commander' to 'startunit'
    data['startunit'] = data['commander']
    data['commander'] = nil
    index = index + 1
    luaSides[index] = data -- lua indices start at 1
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

return luaSides

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
