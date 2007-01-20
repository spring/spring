--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    unit_metal_maker.lua
--  brief:   controls metal makers to minimize energy stalls
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "MetalMakers",
    desc      = "Controls metal makers to minimize energy stalls",
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


local  ON_LIMIT = 0.7
local OFF_LIMIT = 0.3

local metalMakers = {}

local currentState = true


--------------------------------------------------------------------------------

local function SetMetalMakers(state)
  currentState = state

  local selUnits = Spring.GetSelectedUnits()

  Spring.SelectUnitsByKeys(metalMakers)
  
  local numState = currentState and 1 or 0
  Spring.GiveOrder( CMD_ONOFF, { numState }, {} )
  
  Spring.SelectUnitsByValues(selUnits)
end


--------------------------------------------------------------------------------

function widget:Initialize()
  local units = Spring.GetTeamUnits(Spring.GetMyTeamID())
  for _,uid in ipairs(units) do
    local udid = Spring.GetUnitDefID(uid)
    local ud = UnitDefs[udid]
    if (ud.isMetalMaker) then
      metalMakers[uid] = true
      print("Added metal maker: "..uid)
    end
  end
  SetMetalMakers(true)
end


function widget:Update(deltaTime)
  local now, max = Spring.GetTeamResources(Spring.GetMyTeamID(), "energy")
  local frac = (now / max)
  
  if (currentState and (frac < OFF_LIMIT)) then
    SetMetalMakers(false)
  elseif (not currentState and (frac > ON_LIMIT)) then
    SetMetalMakers(true)
  end
end


function widget:UnitFinished(unitID, unitDefID, unitTeam)
  if (unitTeam ~= Spring.GetMyTeamID()) then
    return
  end
  local ud = UnitDefs[unitDefID]
  if (ud.isMetalMaker) then
    print("Added metal maker: "..unitID)
    metalMakers[unitID] = true
    -- set the new unit to our currentState
    local tmpMakers = metalMakers
    metalMakers = { [unitID] = true }
    SetMetalMakers(currentState)
    metalMakers = tmpMakers
  end
end


function widget:UnitDestroyed(unitID, unitDefID, unitTeam)
  if (metalMakers[unitID]) then
    print("Removed metal maker: "..unitID)
  end
  metalMakers[unitID] = nil
end


function widget:UnitTaken(unitID, unitDefID, unitTeam)
  widget:UnitDestroyed(unitID, unitDefID)
end


function widget:UnitGiven(unitID, unitDefID, unitTeam)
  widget:UnitFinished(unitID, unitDefID, unitTeam)
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
