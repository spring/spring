--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    setupdefs.lua
--  brief:   setup some custom UnitDefs parameters,
--           and UnitDefNames, FeatureDefNames, WeaponDefNames
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------


-- Global table to map unitDef names to unitDef tables
UnitDefNames = {}


for _,ud in pairs(UnitDefs) do

  -- add to the name map
  UnitDefNames[ud.name] = ud

  -- set the cost value  (same as shown in the tooltip)
  ud.cost = ud.metalCost + (ud.energyCost / 60.0)

  -- add the custom weapons based parameters
  ud.hasShield      = false
  ud.canParalyze    = false
  ud.canStockpile   = false
  ud.canAttackWater = false
  for _,wt in ipairs(ud.weapons) do
    local wd = WeaponDefs[wt.weaponDef]
    if (wd) then
      if (wd.isShield)    then ud.hasShield      = true end
      if (wd.paralyzer)   then ud.canParalyze    = true end
      if (wd.stockpile)   then ud.canStockpile   = true end
      if (wd.waterWeapon) then ud.canAttackWater = true end
    end
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

FeatureDefNames = {}

for _, fd in pairs(FeatureDefs) do
  FeatureDefNames[fd.name] = fd
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

WeaponDefNames = {}

for _, wd in pairs(WeaponDefs) do
  WeaponDefNames[wd.name] = wd
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
