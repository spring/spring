--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    unitdefs.lua
--  brief:   unitdef parser
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

if (FBIparser) then
  return FBIparser
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local TDF = TDFparser or VFS.Include('gamedata/parse_tdf.lua')
local SND = SNDparser or VFS.Include('gamedata/parse_snd.lua')


--------------------------------------------------------------------------------

local soundTypes = {}


local soundTypes, err = SND.Parse('gamedata/sound.tdf')
if (soundTypes == nil) then
  Spring.Echo('could not load sound data: ' .. err)
  soundTypes = {}
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function SetupWeapons(tdf)
  local weapons = {}

  for w = 1, 16 do
    local name = tdf['weapon' .. w]
    if (name == nil) then
      if (w > 3) then
        break
      end
    else
      local weapon = {}

      weapon.name = name

      local btc = tdf['badtargetcategory'  .. w]
      if (w == 1) then
        btc = tdf['wpri_badtargetcategory'] or btc
      elseif (w == 2) then
        btc = tdf['wsec_badtargetcategory'] or btc
      elseif (w == 3) then
        btc = tdf['wspe_badtargetcategory'] or btc
      end
      weapon.badtargetcategory = btc

      weapon.onlytargetcategory = tdf['onlytargetcategory' .. w]

      weapon.slaveto     = tdf['weaponslaveto'   .. w]
      weapon.maindir     = tdf['weaponmaindir'   .. w]
      weapon.fuelusage   = tdf['weaponfuelusage' .. w]
      weapon.maxangledir = tdf['maxangledif'     .. w]

      weapons[w] = weapon
    end
  end

  if (next(weapons)) then
    tdf.weapons = weapons
  end

  return tdf
end


--------------------------------------------------------------------------------

local function SetupSounds(tdf, soundTypes)
  local soundCat = tdf.soundcategory
  if (soundCat) then
    soundCat = string.lower(soundCat)
    local soundTable = soundTypes[soundCat]
    if (soundTable) then
      tdf.sounds = soundTable
    end
  end
  return tdf
end


--------------------------------------------------------------------------------

local function SetupSpecialEffects(tdf)
  local sfx = tdf.sfxtypes
  if (type(sfx) ~= 'table') then
    return tdf
  end

  local exps = {}
  local e = 0
  while (true) do
    local name = sfx['explosiongenerator' .. e]
    if (name) then
      e = e + 1
      exps[e] = name
    else
      break
    end
  end

  sfx.explosiongenerators = exps

  return tdf
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function ParseFBI(filename)
  local tdf, err = TDF.Parse(filename)
  if (tdf == nil) then
    return nil, err
  end
  if (tdf.unitinfo == nil) then
    return nil, "no [UNITINFO] table"
  end
  tdf = tdf.unitinfo
  
  SetupWeapons(tdf)

  SetupSounds(tdf, soundTypes)

  SetupSpecialEffects(tdf)

  return tdf
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

FBIparser = {
  Parse = ParseFBI
}

return FBIparser

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
