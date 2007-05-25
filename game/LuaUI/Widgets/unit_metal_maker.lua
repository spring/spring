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

local  ON_METAL_LIMIT  = 0.8
local OFF_METAL_LIMIT  = 0.9
local  ON_ENERGY_LIMIT = 0.7
local OFF_ENERGY_LIMIT = 0.3
local metalMakers = {}

local currentState = true


--------------------------------------------------------------------------------

local function SetMetalMakers(state)
  currentState = state
  local numState = currentState and 1 or 0
  Spring.GiveOrderToUnitMap(metalMakers, CMD.ONOFF, { numState }, {} )
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
  local mNow, mMax = Spring.GetTeamResources(Spring.GetMyTeamID(), "metal")
  local eNow, eMax = Spring.GetTeamResources(Spring.GetMyTeamID(), "energy")
  local mFrac = (mNow / mMax)
  local eFrac = (eNow / eMax)
  
  if (currentState) then
    if ((eFrac < OFF_ENERGY_LIMIT) or (mFrac > OFF_METAL_LIMIT)) then
      SetMetalMakers(false)
    end
  else 
    if ((eFrac > ON_ENERGY_LIMIT) and (mFrac < ON_METAL_LIMIT)) then
      SetMetalMakers(true)
    end
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


function widget:UnitTaken(unitID, unitDefID, unitTeam, newTeam)
  widget:UnitDestroyed(unitID, unitDefID)
end


function widget:UnitGiven(unitID, unitDefID, unitTeam, oldTeam)
  widget:UnitFinished(unitID, unitDefID, unitTeam)
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
