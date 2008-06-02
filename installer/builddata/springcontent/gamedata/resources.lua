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

local resources, err = TDF.Parse('gamedata/resources.tdf')

--------------------------------------------------------------------------------


if (resources == nil) then
   -- load the defaults
	resources = {
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
	resources = resources.resources

	local function MakeArray(t, prefix)
		if (type(t) ~= 'table') then
			return nil
		end

		local sorted = {}
		for k,v in pairs(t) do
			if ((type(k) == 'string') and (type(v) == 'string')) then
				local s, e, p, num = k:find('(%D*)(%d*)')
				num = tonumber(num)
				if ((p == prefix) and num) then
					table.insert(sorted, { num, v })
				end
			end
		end
		table.sort(sorted, function(a, b)
			if (a[1] < b[1]) then return true  end
			if (b[1] < a[1]) then return false end
			return (a[2] < b[2])
		end)

		local newTable = {}
		for _,numVal in ipairs(sorted) do
			table.insert(newTable, numVal[2])
		end
		return newTable
	end

	local gfx = resources.graphics
	gfx.smoke    = MakeArray(gfx.smoke,    'smoke')
	gfx.scars    = MakeArray(gfx.scars,    'scar')
	gfx.caustics = MakeArray(gfx.caustics, 'caustic')
end

--------------------------------------------------------------------------------

return resources

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
