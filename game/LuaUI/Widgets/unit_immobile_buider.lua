--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    unit_immobile_buider.lua
--  brief:   sets immobile builders to ROAMING, and gives them a PATROL order
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "ImmobileBuilder",
    desc      = "Sets immobile builders to ROAM, with a PATROL command",
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


local function SetupUnit(unitID)
  -- set immobile builders (nanotowers) to the ROAM movestate,
  -- and give them a PATROL order (does not matter where, afaict)
  local x, y, z = Spring.GetUnitPosition(unitID)
  Spring.GiveOrderToUnit(unitID, CMD_STOP, {}, {})
  Spring.GiveOrderToUnit(unitID, CMD_MOVE_STATE, { 2 }, {})
  Spring.GiveOrderToUnit(unitID, CMD_PATROL, { x + 25, y, z - 25 }, {})
end


function widget:Initialize()
  for _,unitID in ipairs(Spring.GetTeamUnits(Spring.GetMyTeamID())) do
    local unitDefID = Spring.GetUnitDefID(unitID)
    local ud = unitDefID and UnitDefs[unitDefID] or nil
    if (ud and ud.builder and not ud.canMove) then
      SetupUnit(unitID)
    end
  end
end


function widget:UnitCreated(unitID, unitDefID, unitTeam)
  if (unitTeam ~= Spring.GetMyTeamID()) then
    return
  end
  local ud = UnitDefs[unitDefID]
  if (ud and ud.builder and not ud.canMove) then
    SetupUnit(unitID)
  end
end


function widget:UnitGiven(unitID, unitDefID, unitTeam)
  widget:UnitCreated(unitID, unitDefID, unitTeam)
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
