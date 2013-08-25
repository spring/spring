--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    armordefs.lua
--  brief:   armor.txt lua parser
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------


local tdfFile = 'armor.txt'
if (not VFS.FileExists(tdfFile)) then
  return {}
end


local TDF = VFS.Include('gamedata/parse_tdf.lua')
local armorDefs, err = TDF.Parse(tdfFile)
if (armorDefs == nil) then
  error('Error parsing armor.txt: ' .. err)
end

for categoryName, categoryTable in pairs(armorDefs) do
  local t = {}
  -- convert all ArmorDef <key,value> subtables to
  -- array-format (engine expects this as of 95.0;
  -- values were never used and had no meaning)
  for unitName,_ in pairs(categoryTable) do
    t[#t + 1] = unitName
  end

  armorDefs[categoryName] = t
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

return armorDefs

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
