--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    icontypes.lua
--  brief:   icontypes.tdf lua parser
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------


local tdfFile = 'gamedata/icontypes.tdf'
if (not VFS.FileExists(tdfFile)) then
  return {}
end


local TDF = VFS.Include('gamedata/parse_tdf.lua')
local iconTypes, err = TDF.Parse(tdfFile)
if (iconTypes == nil) then
  error('Error parsing icontypes.tdf: ' .. err)
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

return iconTypes.icontypes

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
