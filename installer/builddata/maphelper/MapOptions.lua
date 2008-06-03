
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

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Map config parsing utilities
--

local mapInfo = VFS.Include('maphelper/mapinfo.lua')

if (not mapInfo.autooptions) then
  return {}
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Option creation utilities
--

local function ItemNumbers(min, max, step)
  local t = {}
  for i = min,max,step do
    t[#t + 1] = { key = i }
    -- 'name' is set to 'key'  when missing
    -- 'desc' is set to 'name' when missing
  end
  return t
end


local function BoolOption(key, name, desc, value, default)
  if (value == nil) then
    value = default
  end
  local opt = {
    type = 'bool',
    key = key, name = name, desc = desc,
    def = value
  }
  return opt
end


local function NumberOption(key, name, desc, value, default, min, max, step)
  if (value == nil) then
    value = default
  end
  local opt = {
    type = 'number',
    key = key, name = name, desc = desc,
    def = value, min = min, max = max, step = step
  }
  return opt
end


-- a 'list' option based on numbers
local function RangeOption(key, name, desc, value, default, min, max, step)
  if (value == nil) then
    value = default
  end
  local opt = {
    type = 'number',
    key = key, name = name, desc = desc,
    def = value, min = min, max = max, step = step
  }
  return opt
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Get our defaults from the map config file  (if available)
--

local defGravity  = mapInfo.gravity         or 130.0
local defMaxMetal = mapInfo.maxmetal        or 0.02
local defTidal    = mapInfo.tidalstrength   or 0.0
local defMexRad   = mapInfo.extractorradius or 500.0

local atmo = mapInfo.atmosphere or {}
local defMinWind  = atmo.minwind or 5.0
local defMaxWind  = atmo.maxwind or 25.0


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Example MapOptions.lua 
--

local options = {
  BoolOption(
    'softground', 'SoftGround', 'Allow for terrain height map to be adjusted',
    mapInfo.hardness, true
  ),
  NumberOption(
    'gravity', 'Gravity', 'World gravity',
    mapInfo.gravity, 130, 1.0, 1000.0, 10.0
  ),
  {
    key = 'softground',
    name = 'SoftGround',
    desc = 'Allow for terrain height map to be adjusted',
    type = 'bool',
    def = true,
  },
  {
    key = 'gravity',
    name = 'Gravity',
    desc = 'World gravity',
    type = 'number',
    def = defGravity,
    min = 1.0,
    max = 1000.0,
    step = 10.0,
  },
  {
    key = 'windmin',
    name = 'Wind Minimum',
    desc = 'Controls the minimum amount of wind',
    type = 'number',
    def = defMinWind,
    min = 0.0,
    max = 250.0,
    step = 5.0,
  },
  {
    key = 'windmin2',
    name = 'Wind Minimum',
    desc = 'Controls the minimum amount of wind',
    type = 'list',
    def = defMinWind, items = ItemNumbers(0, 35, 5),
  },
  {
    key = 'windmax',
    name = 'Wind Maximum',
    desc = 'Controls the maximum amount of wind',
    type = 'number',
    def = defMaxWind,
    min = 0.0,
    max = 250.0,
    step = 5.0,
  },
  {
    key = 'tidal',
    name = 'Tidal',
    desc = 'Controls the amount of tidal generation',
    type = 'number',
    def = defTidal,
    min = 0.0,
    max = 250.0,
    step = 5.0,
  },

--[[
  {
    key    = 'string_opt',
    name   = 'String Option',
    desc   = 'an unused string option',
    type   = 'string',
    def    = 'BiteMe',
    maxlen = 12,
  },
--]]
}

return options
