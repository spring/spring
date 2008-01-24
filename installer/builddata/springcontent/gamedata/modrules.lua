--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    modrules.lua
--  brief:   modrules.tdf lua parser
--  author:  Dave Rodgers
--  notes:   Spring.GetModOptions() is available
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local TDF = VFS.Include('gamedata/parse_tdf.lua')

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

if (not VFS.FileExists('gamedata/modrules.tdf')) then
  return false
end

local modrules, err = TDF.Parse('gamedata/modrules.tdf')
if (modrules == nil) then
  error('Error parsing modrules.tdf: ' .. err)
end

--------------------------------------------------------------------------------

return modrules

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
