--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    defs.lua
--  brief:   entry point for unitdefs, featuredefs, and weapondefs parsing
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function Include(...)
  return pcall(VFS.Include, ...)
end

--------------------------------------------------------------------------------

DEFS = {}

--------------------------------------------------------------------------------


-- UnitDefs
local success, unitDefs = Spring.TimeCheck('UNITDEFS', Include, 'gamedata/unitdefs.lua')
if (not success) then
  Spring.Echo("Failed to load unit definitions")
  error(unitDefs)
end
DEFS.UnitDefs = unitDefs


-- FeatureDefs
local success, featureDefs = Include('gamedata/featuredefs.lua')
if (not success) then
  Spring.Echo("Failed to load feature definitions")
  error(featureDefs)
end
DEFS.FeatureDefs = featureDefs


-- WeaponDefs
local success, weaponDefs = Include('gamedata/weapondefs.lua')
if (not success) then
  Spring.Echo("Failed to load weapon definitions")
  error(weaponDefs)
end
DEFS.WeaponDefs = weaponDefs


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  NOTE: the keys have to be lower case
--

return {
  unitdefs    = unitDefs,
  featuredefs = featureDefs,
  weapondefs  = weaponDefs,
}

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
