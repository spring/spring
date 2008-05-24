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
    enabled   = false  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------


local commanderUnitDefs = {}


--------------------------------------------------------------------------------

local iconDir = 'LuaUI/Icons/'

local unitIcons = {
--	{ name = 'default',
		-- Missing parameters get evualeted to the following by Spring:
		--   bitmap = <standard radar dot>
		--   size = 1.0,
		--   radiusadjust = false,
		--   distance = 1.0,
--	},
	{ name = 'default',             size = 1.0, texture = ''                            },
	{ name = 'star.user',           size = 1.5, texture = iconDir..'star.png'           },
	{ name = 'star-dark.user',      size = 1.4, texture = iconDir..'star-dark.png'      },
	{ name = 'tri-up.user',         size = 1.5, texture = iconDir..'tri-up.png'         },
	{ name = 'tri-down.user',       size = 1.5, texture = iconDir..'tri-down.png'       },
	{ name = 'diamond.user',        size = 1.0, texture = iconDir..'diamond.png'        },
	{ name = 'square.user',         size = 1.0, texture = iconDir..'square.png'         },
	{ name = 'square_+.user',       size = 1.0, texture = iconDir..'square_+.png'       },
	{ name = 'square_x.user',       size = 1.0, texture = iconDir..'square_x.png'       },
	{ name = 'm.user',              size = 1.0, texture = iconDir..'m.png'              },
	{ name = 'e.user',              size = 1.0, texture = iconDir..'e.png'              },
	{ name = 'hemi-up.user',        size = 1.2, texture = iconDir..'hemi-up.png'        },
	{ name = 'hemi-down.user',      size = 1.2, texture = iconDir..'hemi-down.png'      },
	{ name = 'cross.user',          size = 1.2, texture = iconDir..'cross.png'          },
	{ name = 'hourglass.user',      size = 1.4, texture = iconDir..'hourglass.png'      },
	{ name = 'hourglass-side.user', size = 1.4, texture = iconDir..'hourglass-side.png' },
	{ name = 'sphere.user',         size = 1.0, texture = iconDir..'sphere.png'         },
	{ name = 'triangle-down.user',  size = 1.0, texture = iconDir..'triangle-down.png'  },
	{ name = 'triangle-up.user',    size = 1.0, texture = iconDir..'triangle-up.png'    },
	{ name = 'x.user',              size = 1.4, texture = iconDir..'x.png'              },
}


--------------------------------------------------------------------------------

function widget:Update(deltaTime)
  -- animated Commander(s) icon
  if (commanderUnitDefs) then
    local timer = widgetHandler:GetHourTimer()
    local iconName
    if (math.mod(timer, 0.5) > 0.25) then
      iconName = "star.user"
    else
      iconName = "star-dark.user"
    end
    for _,udid in ipairs(commanderUnitDefs) do
      Spring.SetUnitDefIcon(udid, iconName)
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
  for _,icon in pairs(unitIcons) do
    Spring.FreeUnitIcon(icon.name)
  end
end


--------------------------------------------------------------------------------
function widget:Initialize()
  -- add the unit icons
  for _,icon in pairs(unitIcons) do
    Spring.AddUnitIcon(icon.name, icon.texture, icon.size)
  end

  -- Setup the unitdef icons
  for udid,ud in pairs(UnitDefs) do
    if (ud ~= nil) then
      if (ud.origIconType == nil) then
        ud.origIconType = ud.iconType
      end

      ud.weaponCount = #ud.weapons

      if (ud.isCommander) then
        -- commanders
        table.insert(commanderUnitDefs, udid) -- save for animation
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
        -- FIXME: allow unknowns to use mod icons?
        Spring.SetUnitDefIcon(udid, "default")
      end
    end
  end
end


--------------------------------------------------------------------------------

