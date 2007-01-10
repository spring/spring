--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    unit_stockpile.lua
--  brief:   adds 100 builds to all new units that can stockpile
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "Stockpiler",
    desc      = "Automatically adds 100 stockpile builds to new units",
    author    = "trepan",
    date      = "Jan 8, 2007",
    license   = "GNU GPL, v2 or later",
    layer     = 0,
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
    if (ud.canStockpile) then
      -- give stockpilers 5 units to build
      GiveUnitOrders(unitID, function ()
      	Spring.GiveOrder( CMD_STOCKPILE, { }, { "ctrl", "shift" } )
      end)
    end
  end
end


--------------------------------------------------------------------------------
