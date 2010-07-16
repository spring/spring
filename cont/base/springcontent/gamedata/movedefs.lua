--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    movedefs.lua
--  brief:   moveinfo.tdf lua parser
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------


local tdfFile = 'gamedata/moveinfo.tdf'
if (not VFS.FileExists(tdfFile)) then
  return {}
end


local TDF = VFS.Include('gamedata/parse_tdf.lua')
local moveTdf, err = TDF.Parse(tdfFile)
if (moveTdf == nil) then
  error('Error parsing moveinfo.tdf: ' .. err)
end

local moveInfo = {}
local i = 0
while (true) do
  local move = moveTdf['class' .. i]
  if (type(move) ~= 'table') then
    break
  end
  i = i + 1
  moveInfo[i] = move
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

return moveInfo

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
