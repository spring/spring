--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    unitdefs.lua
--  brief:   setup some custom UnitDefs parameters
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------


-- Global table to map unitDef name to unitDef tables
UnitDefNames = {}


for _,ud in pairs(UnitDefs) do

  -- add to the name map
  UnitDefNames[ud.name] = ud

  -- add the custom parameters
  ud.canStockpile   = false
  ud.hasShield      = false
  ud.canParalyze    = false
  ud.canAttackWater = false
  for _,wt in ipairs(ud.weapons) do
    local wd = WeaponDefs[wt.weaponDef]
    if (wd) then
      if (wd.stockpile)   then ud.canStockpile   = true end
      if (wd.isShield)    then ud.hasShield      = true end
      if (wd.paralyzer)   then ud.canParalyze    = true end
      if (wd.waterWeapon) then ud.canAttackWater = true end
    end
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
