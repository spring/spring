--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    cmd_factoryclear.lua
--  brief:   clear the build orders for all selected factories
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "FactoryClear",
    desc      = "Adds '/luaui facclear' to clear factories orders",
    author    = "trepan",
    date      = "Jan 8, 2007",
    license   = "GNU GPL, v2 or later",
    layer     = 0,
    enabled   = false  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Factory Multi-Queueing
--

function widget:TextCommand(command)
  if (command == "facclear") then
    ClearFactoryQueues()
    return true
  end
  return false
end   


--------------------------------------------------------------------------------

local function RemoveBuildOrders(unitID, buildDefID, count)
  local opts = {}
  while (count > 0) do
    if (count >= 100) then
      opts = { "right", "ctrl", "shift" }
      count = count - 100
    elseif (count >= 20) then
      opts = { "right", "ctrl" }
      count = count - 20
    elseif (count >= 5) then
      opts = { "right", "shift" }
      count = count - 5
    else    
      opts = { "right" }
      count = count - 1
    end    
    Spring.GiveOrderToUnit(unitID, -buildDefID, {}, opts)
  end
end


function ClearFactoryQueues()
  local udTable = Spring.GetSelectedUnitsSorted()
  udTable.n = nil
  for udidFac,uTable in pairs(udTable) do
    local ud = UnitDefs[udidFac]
    if ((ud ~= nil) and ud.isFactory) then
      uTable.n = nil
      for uid,x in pairs(uTable) do
        local queue = Spring.GetRealBuildQueue(uid)
        if (queue ~= nil) then
          for udid,buildPair in ipairs(queue) do
            local udid, count = next(buildPair, nil)
            RemoveBuildOrders(uid, udid, count)
          end
        end
      end
    end
  end
end


--------------------------------------------------------------------------------
