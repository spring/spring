--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    resources.lua
--  brief:   resources.tdf lua parser
--  author:  Craig Lawrence, Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local TDF = VFS.Include('gamedata/parse_tdf.lua')

local tdfResources, err = TDF.Parse('gamedata/resources.tdf')

--------------------------------------------------------------------------------


if (tdfResources == nil) then
   -- load the defaults
	tdfResources = {
		graphics = {
			caustics = {
				'caustics/caustic00.jpg',
				'caustics/caustic01.jpg',
				'caustics/caustic02.jpg',
				'caustics/caustic03.jpg',
				'caustics/caustic04.jpg',
				'caustics/caustic05.jpg',
				'caustics/caustic06.jpg',
				'caustics/caustic07.jpg',
				'caustics/caustic08.jpg',
				'caustics/caustic09.jpg',
				'caustics/caustic10.jpg',
				'caustics/caustic11.jpg',
				'caustics/caustic12.jpg',
				'caustics/caustic13.jpg',
				'caustics/caustic14.jpg',
				'caustics/caustic15.jpg',
				'caustics/caustic16.jpg',
				'caustics/caustic17.jpg',
				'caustics/caustic18.jpg',
				'caustics/caustic19.jpg',
				'caustics/caustic20.jpg',
				'caustics/caustic21.jpg',
				'caustics/caustic22.jpg',
				'caustics/caustic23.jpg',
				'caustics/caustic24.jpg',
				'caustics/caustic25.jpg',
				'caustics/caustic26.jpg',
				'caustics/caustic27.jpg',
				'caustics/caustic28.jpg',
				'caustics/caustic29.jpg',
				'caustics/caustic30.jpg',
				'caustics/caustic31.jpg',
			},
			smoke = {
				'smoke/smoke00.tga',
				'smoke/smoke01.tga',
				'smoke/smoke02.tga',
				'smoke/smoke03.tga',
				'smoke/smoke04.tga',
				'smoke/smoke05.tga',
				'smoke/smoke06.tga',
				'smoke/smoke07.tga',
				'smoke/smoke08.tga',
				'smoke/smoke09.tga',
				'smoke/smoke10.tga',
				'smoke/smoke11.tga',
			},
			scars = {
				'scars/scar1.bmp',
				'scars/scar2.bmp',
				'scars/scar3.bmp',
				'scars/scar4.bmp',
			},
			trees = {
				bark		=	'Bark.bmp',
				leaf		=	'bleaf.bmp',
				gran1		=	'gran.bmp',
				gran2		=	'gran2.bmp',
				birch1	=	'birch1.bmp',
				birch2	=	'birch2.bmp',
				birch3	=	'birch3.bmp',
			},
			maps = {
				detailtex	=	'detailtex2.bmp',
				watertex	=	'ocean.jpg',
			},
			groundfx = {
				groundflash	=	'groundflash.tga',
				groundring	=	'groundring.tga',
				seismic			=	'circles.tga',
			},
			projectiletextures = {
				circularthingy	=	'circularthingy.tga',
				laserend				=	'laserend.tga',
				laserfalloff		=	'laserfalloff.tga',
				randdots				=	'randdots.tga',
				smoketrail			=	'smoketrail.tga',
				wake						=	'wake.tga',
				flare						=	'flare.tga',
				explo						=	'explo.tga',
				explofade				=	'explofade.tga',
				heatcloud				=	'explo.tga',
				flame						=	'flame.tga',
				muzzleside			=	'muzzleside.tga',
				muzzlefront			=	'muzzlefront.tga',
				largebeam				=	'largelaserfalloff.tga',
			},
		}
	}

else
	tdfResources = tdfResources.resources
	local function LuaNumbers(tablename, loopstart, loopend, thestring)
		local tabledata = tdfResources.graphics[tablename]
		if (type(tabledata) == 'table') then
			for i=loopstart,loopend do
				if (i > 9) then
					thestring = string.sub(thestring,-string.len(thestring), -1) -- chop off the '0's
				end
				-- deal with scars starting at 1
				tabledata[i-(loopstart-1)] = tabledata[thestring .. tostring(i)]
				tabledata[thestring .. tostring(i)] = nil
			end
		end
	end
	LuaNumbers('smoke', 0, 12, 'smoke0')
	LuaNumbers('scars', 1, 4, 'scar')
	LuaNumbers('caustics', 0, 32, 'caustic0')
end

--------------------------------------------------------------------------------

return tdfResources

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
