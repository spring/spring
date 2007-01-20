--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    unit_factory_guard.lua
--  brief:   assigns new builder units to guard their source factory
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "FactoryGuard",
    desc      = "Assigns new builders to assist their source factory",
    author    = "trepan",
    date      = "Jan 8, 2007",
    license   = "GNU GPL, v2 or later",
    layer     = 0,
    enabled   = false  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

include("spring.h.lua")


local OrderUnit = Spring.GiveOrderToUnit


function widget:UnitFromFactory(unitID, unitDefID, facID, facDefID, userOrders)
  -- is the builder a factory?
  if (userOrders) then
    return
  end
  local bd = UnitDefs[facDefID]
  if (not (bd and bd.isFactory)) then
    return
  end
  -- can this unit assist?
  local ud = UnitDefs[unitDefID]
  if ((ud ~= nil) and ud.builder and ud.canAssist and
      (Spring.GetUnitTeam(unitID) == Spring.GetMyTeamID())) then
    -- set builders to guard/assist their factories
    local x, y, z = Spring.GetUnitPosition(facID)
    OrderUnit(unitID, CMD_MOVE,  { x + 100, y, z + 100 }, { "" })
    OrderUnit(unitID, CMD_MOVE,  { x + 100, y, z },       { "shift" })
    OrderUnit(unitID, CMD_GUARD, { facID },               { "shift" })
  end
end


--------------------------------------------------------------------------------
