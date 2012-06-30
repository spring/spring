--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    parse_fbi.lua
--  brief:   unitdef FBI parser
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
local section = 'parse_fbi.lua'

--------------------------------------------------------------------------------

local soundTypes = {}


local soundTypes, err = SND.Parse('gamedata/sound.tdf')
if (soundTypes == nil) then
  if (false) then -- sound.tdf is no longer required, do not complain
    Spring.Log(section, LOG.ERROR, 'could not load sound data: ' .. err)
  end
  soundTypes = {}
end


--------------------------------------------------------------------------------

local buildOptions = {}


local buildOptions, err = TDF.Parse('gamedata/sidedata.tdf')
buildOptions = buildOptions or {}
buildOptions = buildOptions['canbuild']
buildOptions = buildOptions or {}

do
  local newBuildOptions = {}

  local function AddBuildOptions(unitName, buildTable)
    local sorted = {}
    for buildID, buildName in pairs(buildTable) do
      local idstr = string.lower(buildID)
      local s, e, idnum = string.find(idstr, 'canbuild(%d+)')
      idnum = tonumber(idnum)
      if (idnum) then
        table.insert(sorted, { idnum, string.lower(buildName) })
      end
    end
    table.sort(sorted, function(a, b) return a[1] < b[1] end)
    for i, buildName in ipairs(sorted) do
      sorted[i] = sorted[i][2]
    end
    newBuildOptions[unitName] = sorted
  end

  for unitName, buildTable in pairs(buildOptions) do
    AddBuildOptions(string.lower(unitName), buildTable)
  end

  buildOptions = newBuildOptions
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function SetupWeapons(tdf)
  local weapons = {}

  for w = 1, 32 do
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
      weapon.maxangledif = tdf['maxangledif'     .. w]

      -- clear the old style parameters
      tdf['weapon' .. w]             = nil
      tdf['badtargetcategory'  .. w] = nil
      tdf['wpri_badtargetcategory']  = nil
      tdf['wsec_badtargetcategory']  = nil
      tdf['wspe_badtargetcategory']  = nil
      tdf['onlytargetcategory' .. w] = nil
      tdf['weaponslaveto'   .. w]    = nil
      tdf['weaponmaindir'   .. w]    = nil
      tdf['weaponfuelusage' .. w]    = nil
      tdf['maxangledif'     .. w]    = nil

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
    tdf.soundcategory = nil -- clear the old style parameter
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
    sfx['explosiongenerator' .. e] = nil -- clear the old style parameter
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

local function SetupBuildOptions(tdf)
  if (tdf.unitname == nil) then
    return
  end
  if (tdf.buildOptions) then
    return
  end
  local name = string.lower(tdf.unitname)
  tdf.buildoptions = buildOptions[name]
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

  -- override the unitname, 0.75b2 expects it to be filename derived
  local lowername = string.lower(filename)
  local s, e, basename = string.find(lowername, '([^\\/]+)%....$')
  tdf.unitname = basename

  -- backwards compatible hack for non-standard parameter naming
  if (tdf.init_cloaked ~= nil) then
    if (tdf.initcloaked == nil) then
      tdf.initcloaked = tdf.init_cloaked
    end
    tdf.init_cloaked = nil
  end
  
  SetupWeapons(tdf)

  SetupSounds(tdf, soundTypes)

  SetupSpecialEffects(tdf)

  SetupBuildOptions(tdf)

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
