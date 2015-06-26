--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    weapondefs_post.lua
--  brief:   weaponDef post processing
--  author:  Dave Rodgers
--
--  Copyright (C) 2008.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Per-unitDef weaponDefs
--

local function isbool(x)   return (type(x) == 'boolean') end
local function istable(x)  return (type(x) == 'table')   end
local function isnumber(x) return (type(x) == 'number')  end
local function isstring(x) return (type(x) == 'string')  end

local function tobool(val)
  local t = type(val)
  if (t == 'nil') then
    return false
  elseif (t == 'boolean') then
    return val
  elseif (t == 'number') then
    return (val ~= 0)
  elseif (t == 'string') then
    return ((val ~= '0') and (val ~= 'false'))
  end
  return false
end

local function hs2rgb(h, s)
	--// FIXME? ignores saturation completely
	s = 1

	local invSat = 1 - s

	if (h > 0.5) then h = h + 0.1 end
	if (h > 1.0) then h = h - 1.0 end

	local r = invSat / 2.0
	local g = invSat / 2.0
	local b = invSat / 2.0

	if (h < (1.0 / 6.0)) then
		r = r + s
		g = g + s * (h * 6.0)
	elseif (h < (1.0 / 3.0)) then
		g = g + s
		r = r + s * ((1.0 / 3.0 - h) * 6.0)
	elseif (h < (1.0 / 2.0)) then
		g = g + s
		b = b + s * ((h - (1.0 / 3.0)) * 6.0)
	elseif (h < (2.0 / 3.0)) then
		b = b + s
		g = g + s * ((2.0 / 3.0 - h) * 6.0)
	elseif (h < (5.0 / 6.0)) then
		b = b + s
		r = r + s * ((h - (2.0 / 3.0)) * 6.0)
	else
		r = r + s
		b = b + s * ((3.0 / 3.0 - h) * 6.0)
	end

	return ("%0.3f %0.3f %0.3f"):format(r,g,b)
end

--------------------------------------------------------------------------------

local function BackwardCompability(wdName, wd)
  -- weapon reloadTime and stockpileTime were seperated in 77b1
  if (tobool(wd.stockpile) and (wd.stockpiletime==nil)) then
    wd.stockpiletime = wd.reloadtime
    wd.reloadtime    = 2             -- 2 seconds
  end

  -- auto detect ota weapontypes
  if (wd.weapontype == nil) then
    local rendertype = tonumber(wd.rendertype) or 0

    if (tobool(wd.dropped)) then
      wd.weapontype = "AircraftBomb"
    elseif (tobool(wd.vlaunch)) then
      wd.weapontype = "StarburstLauncher"
    elseif (tobool(wd.beamlaser)) then
      wd.weapontype = "BeamLaser"
    elseif (tobool(wd.isshield)) then
      wd.weapontype = "Shield"
    elseif (tobool(wd.waterweapon)) then
      wd.weapontype = "TorpedoLauncher"
    elseif (wdName:lower():find("disintegrator",1,true)) then
      wd.weaponType = "DGun"
    elseif (tobool(wd.lineofsight)) then
      if (rendertype == 7) then
        wd.weapontype = "LightningCannon"

      -- swta fix (outdated?)
      elseif (wd.model and wd.model:lower():find("laser",1,true)) then
        wd.weapontype = "LaserCannon"

      elseif (tobool(wd.beamweapon)) then
        wd.weapontype = "LaserCannon"
      elseif (tobool(wd.smoketrail)) then
        wd.weapontype = "MissileLauncher"
      elseif (rendertype == 4 and tonumber(wd.color)==2) then
        wd.weapontype = "EmgCannon"
      elseif (rendertype == 5) then
        wd.weapontype = "Flame"
      --elseif(rendertype == 1) then
      --  wd.weapontype = "MissileLauncher"
      else
        wd.weapontype = "Cannon"
      end
    else
      wd.weapontype = "Cannon"
    end
  end

  if (wd.weapontype == "LightingCannon") then
    wd.weapontype = "LightningCannon"
  elseif (wd.weapontype == "AircraftBomb") then
    if (wd.manualbombsettings) then
      wd.reloadtime = tonumber(wd.reloadtime or 1.0)
      wd.burstrate  = tonumber(wd.burstrate or 0.1)

      if (wd.reloadtime < 0.5) then
        wd.burstrate  = math.min(0.2, wd.reloadtime)         -- salvodelay
        wd.burst      = math.floor((1.0 / wd.burstrate) + 1) -- salvosize
        wd.reloadtime = 5.0
      else
        wd.burstrate = math.min(0.4, wd.reloadtime)
        wd.burst     = 2
      end
    end
  end

  if (not wd.rgbcolor) then
    if (wd.weapontype == "Cannon") then
      wd.rgbcolor = "1.0 0.5 0.0"
    elseif (wd.weapontype == "EmgCannon") then
      wd.rgbcolor = "0.9 0.9 0.2"
    else
      local hue = (wd.color or 0) / 255
      local sat = (wd.color2 or 0) / 255
      wd.rgbcolor = hs2rgb(hue, sat)
    end
  end

  if (not wd.craterareaofeffect) then
    wd.craterareaofeffect = tonumber(wd.areaofeffect or 0) * 1.5
  end

  if (tobool(wd.ballistic) or tobool(wd.dropped)) then
    wd.gravityaffected = true
  end

  -- remove the legacy tags (so engine doesn't print warnings)
  wd.rendertype = nil
  wd.dropped = nil
  wd.vlaunch = nil
  wd.beamlaser = nil
  wd.isshield = nil
  wd.lineofsight = nil
  wd.beamweapon = nil
  wd.manualbombsettings = nil
  wd.color = nil
  wd.color2 = nil
  wd.ballistic = nil
end

--------------------------------------------------------------------------------

local function ProcessUnitDef(udName, ud)

  local wds = ud.weapondefs
  if (not istable(wds)) then
    return
  end

  -- add this unitDef's weaponDefs
  for wdName, wd in pairs(wds) do
    if (isstring(wdName) and istable(wd)) then
      local fullName = udName .. '_' .. wdName
      WeaponDefs[fullName] = wd
    end
  end

  -- convert the weapon names
  local weapons = ud.weapons
  if (istable(weapons)) then
    for i = 1, 32 do
      local w = weapons[i]
      if (istable(w)) then
        if (isstring(w.def)) then
          local ldef = string.lower(w.def)
          local fullName = udName .. '_' .. ldef
          local wd = WeaponDefs[fullName]
          if (istable(wd)) then
            w.name = fullName
          end
        end
        w.def = nil
      end
    end
  end

  -- convert the death explosions
  if (isstring(ud.explodeas)) then
    local fullName = udName .. '_' .. ud.explodeas
    if (WeaponDefs[fullName]) then
      ud.explodeas = fullName
    end
  end
  if (isstring(ud.selfdestructas)) then
    local fullName = udName .. '_' .. ud.selfdestructas
    if (WeaponDefs[fullName]) then
      ud.selfdestructas = fullName
    end
  end
end

--------------------------------------------------------------------------------

local function ProcessWeaponDef(wdName, wd)

  -- backward compability
  BackwardCompability(wdName, wd)
end

--------------------------------------------------------------------------------

-- Process the unitDefs
local UnitDefs = DEFS.unitDefs

for udName, ud in pairs(UnitDefs) do
  if (isstring(udName) and istable(ud)) then
    ProcessUnitDef(udName, ud)
  end
end


for wdName, wd in pairs(WeaponDefs) do
  if (isstring(wdName) and istable(wd)) then
    ProcessWeaponDef(wdName, wd)
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
