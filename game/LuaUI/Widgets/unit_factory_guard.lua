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

local function ClearGroup(unitID, factID)
  -- clear the unit's group if it's the same as the factory's
  local unitGroup = Spring.GetUnitGroup(unitID)
  if (not unitGroup) then
    return
  end
  local factGroup = Spring.GetUnitGroup(factID)
  if (not factGroup) then
    return
  end
  if (unitGroup == factGroup) then
    Spring.SetUnitGroup(unitID, -1)
  end
end


local function GuardFactory(unitID, unitDefID, factID, factDefID)
  -- is this a factory?
  local fd = UnitDefs[factDefID]
  if (not (fd and fd.isFactory)) then
    return 
  end

  -- can this unit assist?
  local ud = UnitDefs[unitDefID]
  if (not (ud and ud.builder and ud.canAssist)) then
    return
  end

  local x, y, z = Spring.GetUnitPosition(factID)
  if (not x) then
    return
  end

  local radius = Spring.GetUnitRadius(factID)
  if (not radius) then
    return
  end
  local dist = radius * 2

  local facing = Spring.GetUnitBuildFacing(factID)
  if (not facing) then
    return
  end

  -- facing values { S = 0, E = 1, N = 2, W = 3 }  
  local dx, dz -- down vector
  local rx, rz -- right vector
  if (facing == 0) then
    -- south
    dx, dz =  0,  dist
    rx, rz =  dist,  0
  elseif (facing == 1) then
    -- east
    dx, dz =  dist,  0
    rx, rz =  0, -dist
  elseif (facing == 2) then
    -- north
    dx, dz =  0, -dist
    rx, rz = -dist,  0
  else
    -- west
    dx, dz = -dist,  0
    rx, rz =  0,  dist
  end
  
  local OrderUnit = Spring.GiveOrderToUnit

  OrderUnit(unitID, CMD.MOVE,  { x + dx, y, z + dz }, { "" })
  OrderUnit(unitID, CMD.MOVE,  { x + rx, y, z + rz }, { "shift" })
  OrderUnit(unitID, CMD.GUARD, { factID },            { "shift" })
end


--------------------------------------------------------------------------------

function widget:UnitFromFactory(unitID, unitDefID, unitTeam,
                                factID, factDefID, userOrders)
  if (unitTeam ~= Spring.GetMyTeamID()) then
    return -- not my unit
  end
  
  ClearGroup(unitID, factID)

  if (userOrders) then
    return -- already has user assigned orders
  end

  GuardFactory(unitID, unitDefID, factID, factDefID)
end


--------------------------------------------------------------------------------
