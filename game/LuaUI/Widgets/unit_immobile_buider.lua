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
    date      = "Jan 11, 2007",
    license   = "GNU GPL, v2 or later",
    drawLayer = 0,
    enabled   = true  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

include("spring.h.lua")


local function SelectedString()
  local s = ""
  uidTable = Spring.GetSelectedUnits()
  uidTable.n = nil  --  or use ipairs
  for k,v in pairs(uidTable) do
   	s = s .. ' +' .. v
  end
  return s
end


local function GiveUnitOrders(unitID, func)
  local selstr = SelectedString()
  Spring.SendCommands({ "selectunits clear +" .. unitID })
  func(unitID)
  Spring.SendCommands({ "selectunits clear" .. selstr })
end


function widget:UnitCreated(unitID, unitDefID)
  local ud = UnitDefs[unitDefID]
  if ((ud ~= nil) and
      (Spring.GetUnitTeam(unitID) == Spring.GetMyTeamID())) then
    if (ud.builder and not ud.canMove) then
      -- set immobile builders (nanotowers) to the ROAM movestate,
      -- and give them a PATROL order (does not matter where, afaict)
      GiveUnitOrders(unitID, function ()
        x, y, z = Spring.GetUnitPosition(unitID)
        Spring.GiveOrder( CMD_MOVE_STATE, { 2 }, { } )
        Spring.GiveOrder( CMD_PATROL, { x + 25, y, z - 25 }, { } )
      end)
    end   
  end
end
