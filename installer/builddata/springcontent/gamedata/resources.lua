--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    resources.lua
--  brief:   resuources.tdf lua parser
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

if (tdfResources) then
  return tdfResources.resources
end


-- default resources
local defResources = {
  graphics = {
    smoke = {
      smoke00 = 'smoke/smoke00.tga',
      smoke01 = 'smoke/smoke01.tga',
      smoke02 = 'smoke/smoke02.tga',
      smoke03 = 'smoke/smoke03.tga',
      smoke04 = 'smoke/smoke04.tga',
      smoke05 = 'smoke/smoke05.tga',
      smoke06 = 'smoke/smoke06.tga',
      smoke07 = 'smoke/smoke07.tga',
      smoke08 = 'smoke/smoke08.tga',
      smoke09 = 'smoke/smoke09.tga',
      smoke10 = 'smoke/smoke10.tga',
      smoke11 = 'smoke/smoke11.tga',
    },
    scars = {
      scar1 = 'scars/scar1.bmp',
      scar2 = 'scars/scar2.bmp',
      scar3 = 'scars/scar3.bmp',
      scar4 = 'scars/scar4.bmp',
    },
    trees = {
      bark   = 'Bark.bmp',
      leaf   = 'bleaf.bmp',
      gran1  = 'gran.bmp',
      gran2  = 'gran2.bmp',
      birch1 = 'birch1.bmp',
      birch2 = 'birch2.bmp',
      birch3 = 'birch3.bmp',
    },
    maps = {
      detailtex = 'detailtex2.bmp',
      watertex  = 'ocean.jpg',
    },
    groundfx = {
      groundflash = 'groundflash.tga',
      groundring  = 'groundring.tga',
      seismic     = 'circles.tga',
    },
    projectiletextures = {
      circularthingy = 'circularthingy.tga',
      laserend       = 'laserend.tga',
      laserfalloff   = 'laserfalloff.tga',
      randdots       = 'randdots.tga',
      smoketrail     = 'smoketrail.tga',
      wake           = 'wake.tga',
      flare          = 'flare.tga',
      explo          = 'explo.tga',
      explofade      = 'explofade.tga',
      heatcloud      = 'explo.tga',
      flame          = 'flame.tga',
      muzzleside     = 'muzzleside.tga',
      muzzlefront    = 'muzzlefront.tga',
      largebeam      = 'largelaserfalloff.tga',
    },
  },
}


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

return defResources

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
