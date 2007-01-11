--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    cmd_circle.lua
--  brief:   adds a command to make circular unit formations
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "CircleFormation",
    desc      = "Adds '/luaui circle' to position mobile units",
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


function widget:TextCommand(command)
  if (command ~= 'circle') then
    return false
  else
    CircleUnits()
	end
	return true
end
        

function CircleUnits()
  local units = Spring.GetSelectedUnits()
  table.sort(units)
  local count = units.n
  units.n = nil

  if (count < 2) then return end
  
  local x, y, z = 0.0, 0.0, 0.0
  local radsum = 0.0
  local radii = {}

  for tmp,uid in pairs(units) do
    local ux, uy, uz = Spring.GetUnitPosition(uid)
    x = x + ux
    y = y + uy
    z = z + uz

    local rad = Spring.GetUnitRadius(uid)
    radii[uid] = rad
    radsum = radsum + rad
  end
  x = x / count
  y = y / count
  z = z / count

  local radius0 = (radsum / math.pi) * (count / (count - 1.5))
  local radius1 = radius0 + 50

  local selUnits = Spring.GetSelectedUnits()
  local pi2 = math.pi * 2.0
  local _,first = next(units)  
  
  local arcdist = 0
  for t,uid in ipairs(units) do
    if (uid ~= first) then
      arcdist = arcdist + (0.5 * radii[uid])
    end
    local rads = pi2 * arcdist / radsum
    local ux = x + radius0 * math.sin(rads)  
    local uz = z - radius0 * math.cos(rads)  
    local uy = Spring.GetGroundHeight(ux, uz)
    Spring.SelectUnitsByValues({uid})
    Spring.GiveOrder(CMD_MOVE, {ux, uy, uz}, {})
    arcdist = arcdist + (0.5 * radii[uid])
  end

  Spring.SelectUnitsByValues(selUnits)
  Spring.GiveOrder(CMD_GATHERWAIT, {}, {})

  local arcdist = 0
  for t,uid in ipairs(units) do
    if (uid ~= first) then
      arcdist = arcdist + (0.5 * radii[uid])
    end
    local rads = pi2 * arcdist / radsum
    local ux = x + radius1 * math.sin(rads)  
    local uz = z - radius1 * math.cos(rads)  
    local uy = Spring.GetGroundHeight(ux, uz)
    Spring.SelectUnitsByValues({uid})
    Spring.GiveOrder(CMD_MOVE, {ux, uy, uz}, {"shift"})
    arcdist = arcdist + (0.5 * radii[uid])
  end

  Spring.SelectUnitsByValues(selUnits)

  -- helps to avoid last minute pushing
  --Spring.GiveOrder(CMD_WAIT, {}, {"shift"})
end
