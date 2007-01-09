--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    ico_customicons.lua
--  brief:   controls the custom icons (see usericons.tdf)
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "CustomIcons",
    desc      = "Controls the custom icons defined in usericons.tdf",
    author    = "trepan",
    date      = "Jan 8, 2007",
    license   = "GNU GPL, v2 or later",
    layer     = 0,
    enabled   = true  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------


local commanderUnitDef = -1


--------------------------------------------------------------------------------

function widget:Update(deltaTime)
  -- animated Commander(s) icon
  if (commanderUnitDef >= 0) then
    local timer = widgetHandler:GetHourTimer()
    if (math.mod(timer, 0.5) > 0.25) then
      Spring.SetUnitDefIcon(commanderUnitDef, "star.user")
    else
      Spring.SetUnitDefIcon(commanderUnitDef, "star-dark.user")
    end
  end
end


function widget:Shutdown()
  -- revert our changes
  for udid,ud in pairs(UnitDefs) do
    if ((ud ~= nil) and (ud.origIconType ~= nil)) then
      Spring.SetUnitDefIcon(udid, ud.origIconType)
    end
  end
end


--------------------------------------------------------------------------------


function widget:Initialize()
  -- Setup the unitdef icons
  for udid,ud in pairs(UnitDefs) do
    if (ud ~= nil) then
      if (ud.origIconType == nil) then
        ud.origIconType = ud.iconType
      end

      if (ud.isCommander or (ud.TEDClass == "COMMANDER")) then
        -- commanders
        commanderUnitDef = udid -- save for animation
        Spring.SetUnitDefIcon(udid, "star.user")
      elseif (ud.isFactory) then
        -- factories
        Spring.SetUnitDefIcon(udid, "square_x.user")
      elseif (ud.isBuilder) then
        -- builders
        if ((ud.speed > 0) and ud.canMove) then
          Spring.SetUnitDefIcon(udid, "cross.user")     -- mobile
        else
          Spring.SetUnitDefIcon(udid, "square_+.user")  -- immobile
        end
      elseif (ud.canFly) then
        -- aircraft
        Spring.SetUnitDefIcon(udid, "tri-up.user")
      elseif ((ud.speed <= 0) and ud.hasShield) then
        -- immobile shields
        Spring.SetUnitDefIcon(udid, "hemi-up.user")
      elseif ((ud.extractsMetal > 0) or (ud.makesMetal > 0)) then
        -- metal extractors and makers
        Spring.SetUnitDefIcon(udid, "m.user")
      elseif ((ud.totalEnergyOut > 10) and (ud.speed <= 0)) then
        -- energy generators
        Spring.SetUnitDefIcon(udid, "e.user")
      elseif (ud.isTransport) then
        -- transports
        Spring.SetUnitDefIcon(udid, "diamond.user")
      elseif ((ud.minWaterDepth > 0) and (ud.speed > 0) and (ud.waterline > 10)) then
        -- submarines
        Spring.SetUnitDefIcon(udid, "tri-down.user")
      elseif ((ud.minWaterDepth > 0) and (ud.speed > 0)) then
        -- ships
        Spring.SetUnitDefIcon(udid, "hemi-down.user")
      elseif (((ud.radarRadius > 1) or
               (ud.sonarRadius > 1) or
               (ud.seismicRadius > 1)) and (ud.speed <= 0)) then
        -- sensors
        Spring.SetUnitDefIcon(udid, "hourglass-side.user")
      elseif (((ud.jammerRadius > 1) or
               (ud.sonarJamRadius > 1)) and (ud.speed <= 0)) then
        -- jammers
        Spring.SetUnitDefIcon(udid, "hourglass.user")
      elseif (ud.isBuilding or (ud.speed <= 0)) then
        -- defenders and other buildings
        if (ud.weaponCount <= 0) then
          Spring.SetUnitDefIcon(udid, "square.user")
        else 
          Spring.SetUnitDefIcon(udid, "x.user")
        end
      else
        -- fixme: allow unknowns to use mod icons?
        Spring.SetUnitDefIcon(udid, "default")
      end
    end
  end
end


--------------------------------------------------------------------------------

