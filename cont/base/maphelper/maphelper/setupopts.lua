--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    setupopts.lua
--  brief:   returns a function that creates a MapOptions table
--  author:  Dave Rodgers
--
--  Copyright (C) 2008.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

--  Custom Options Definition Table format

--  NOTES:
--  - using an enumerated table lets you specify the options order

--
--  These keywords must be lowercase for LuaParser to read them.
--
--  key:      the string used in the script.txt
--  name:     the displayed name
--  desc:     the description (could be used as a tooltip)
--  type:     the option type (bool, number, string, list)
--  def:      the default value
--  min:      minimum value for number options
--  max:      maximum value for number options
--  step:     quantization step, aligned to the def value
--  maxlen:   the maximum string length for string options
--  items:    array of item strings for list options
--  scope:    'all', 'player', 'team', 'allyteam'      <<< not supported yet >>>
--

local section = 'setupopts.lua'
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Parse the desired options
--

local function AddBasic(opts)
  table.insert(opts, 'notdeformable')
  table.insert(opts, 'hardness')
  table.insert(opts, 'voidwater')
  table.insert(opts, 'waterdamage')
  table.insert(opts, 'maxmetal')
  table.insert(opts, 'extractorradius')
  table.insert(opts, 'minwind')
  table.insert(opts, 'maxwind')
  table.insert(opts, 'tidalstrength')
end


local function AddAdvanced(opts)
  AddBasic(opts)
end


local function CreateWantedOpts(optSel)
  local wantedOpts = {}
  if (type(optSel) == 'string') then
	-- FIXME
  elseif (type(optSel) == 'boolean') then
    if (optSet) then
      AddBasic(wantedOpts)
    else
      return nil
    end
  elseif (type(optSel) == 'table') then
  
  end
  return wantedOpts
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Option definition utilities
--

local function ItemNumbers(optDef)
  local t = {}
  for i = min,max,step do
    t[#t + 1] = { key = i }
    -- 'name' is set to 'key'  when missing
    -- 'desc' is set to 'name' when missing
  end
  return t
end


local function BoolOption(optDef)
  if (value == nil) then
    value = default
  end
  local opt = {
    type = 'bool',
    key = optDef[1], name = optDef[2], desc = optDef[3],
    def = optDef[4] or optDef[5]
  }
  return opt
end


local function NumberOption(optDef)
  local opt = {
    type = 'number',
    key = optDef[1], name = optDef[2], desc = optDef[3],
    def = optDef[4] or optDef[5],
    min = optDef[6], max = optDef[7], step = optDef[8]
  }
  return opt
end


local function ColorOption(optDef)
  local opt = {
    type = 'string',
    key = optDef[1], name = optDef[2], desc = optDef[3],
    def = optDef[4] or optDef[5],
  }
  return opt
end


local function ParseFloat3(value)
  local t = type(value)
  if (t == 'table') then
    if (tonumber(value[1]) and
        tonumber(value[2]) and
        tonumber(value[3])) then
      return value
    else
      return nil
    end
  elseif (t == 'string') then
    local s, e, v1, v2, v3 = value:find('^%s*(%S+)%s+(%S+)%s+(%S+)%s*')
    v1 = tonumber(v1)
    v2 = tonumber(v2)
    v3 = tonumber(v3)
    if (v1 and v2 and v3) then
      return { v1, v2, v3 }
    else
      return nil
    end
  end
  return nil
end


local function TintColor(color, tint)
  return {
    color[1] * tint[1],
    color[2] * tint[2],
    color[3] * tint[3],
  }
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Option Definitions
--

-- key: option selector key 
--   1: option key
--   2: option name
--   3: option description
--   4: option generator function
--   5: map value
--   6: default value
--   7: min
--   8: max
--   9: step

local function CreateOptionDefs(mapInfo, defValues)

  local optionDefs = {

    -- basics

    notdeformable = {
      'notdeformable', 'NotDeformable', 'Ground can not be deformed',
      BoolOption, mapInfo.hardness, true
    },
    hardness = {
      'hardness', 'Hardness', 'Terrain hardness',
      NumberOption, mapInfo.hardness, defValues.hardness,
      0.0, 10000.0
    },
    voidwater = {
      'voidwater', 'Void Water', 'Draw holes instead of water',
      mapInfo.voidwater, defValues.voidwater
    },
    gravity = {
      'gravity', 'Gravity', 'Map Gravity',
      NumberOption,  mapInfo.gravity, defValues.gravity,
      1.0, 1000.0
    },
    maxmetal = {
      'maxmetal', 'Max Metal', 'Maximum metal density',
      NumberOption, mapInfo.maxmetal, defValues.maxMetal,
      0.0, 10.0
    },
    extractorradius = {
      'extractorradius', 'Extractor Radius',
      'Sets the default metal extractor radius',
      NumberOption, mapInfo.extractorradius, defValues.extractorRadius,
      1.0, 10000.0
    },
    tidalstrength = {
      'tidalstrength', 'Tidal Strength', 'Tidal energy level',
      mapInfo.tidalstrength, defValues.tidalStrength,
      0.0, 1000.0
    },

    -- atmosphere

    windmin = {
      'windmin', 'Wind Minimum', 'Minimum amount of wind',
      NumberOption,
      mapInfo.atmosphere.windmin, defValues.atmosphere.windmin,
      0.0, 1000.0
    },
    windmax = {
      'windmax', 'Wind Maximum', 'Maximum amount of wind',
      NumberOption,
      mapInfo.atmosphere.windmax, defValues.atmosphere.windmax,
      0.0, 1000.0
    },

    -- water

    waterdamage = {
      'waterdamage', 'Water Damage', 'Damage rate for units in the water',
      NumberOption,
      mapInfo.water.damage, defValues.water.damage,
      0.0, 1000.0
    },
  }

  return optionDefs
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function SetupOpts(mapInfo)

  local wantedOpts = CreateWantedOpts(mapInfo.defaultoptions)
  if (not wantedOpts) then
    Spring.Log(section, LOG.ERROR, 'could not parse default map options')
    return {}
  end

  local defaultVals = VFS.Include('maphelper/mapdefaults.lua')
  if (not defaultVals) then
    Spring.Log(section, LOG.ERROR, 'could not load default map values')
    return {}
  end

  local optionDefs = CreateOptionDefs(mapInfo, defaultVals)
  if (not optionDefs) then
    Spring.Log(section, LOG.ERROR, 'could not create map options')
    return {}
  end  

  local options = {}

  for _, opt in ipairs(wantedOpts) do
    local optDef = optionDefs[opt]
    if (optDef) then
      if (type(optDef[4]) ~= 'function') then
        Spring.Log(section, LOG.ERROR, 'bad option definition in MapOptions.lua: ' .. opt)
      else
        table.insert(options, optDef[4](optDef))
      end
    end
  end

  return options
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

return SetupOpts

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
