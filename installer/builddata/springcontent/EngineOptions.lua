--  Custom Options Definition Table format

--  NOTES:
--  - using an enumerated table lets you specify the options order

--
--  These keywords must be lowercase for LuaParser to read them.
--
--  key:      the string used in the script.txt
--  name:     the displayed name
--  desc:     the description (could be used as a tooltip)
--  type:     the option type
--  def:      the default value
--  min:      minimum value for number options
--  max:      maximum value for number options
--  step:     quantization step, aligned to the def value
--  maxlen:   the maximum string length for string options
--  items:    array of item strings for list options
--  scope:    'global', 'player', 'team', 'allyteam'
--

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Example EngineOptions.lua
--

local options =
{
  {
    key    = 'GameMode',
    name   = 'Game end condition',
    desc   = 'Determines what condition triggers the defeat of a player',
    type   = 'list',
    def    = '0',
    items  =
    {
      {
        key  = '0',
        name = 'Kill everything',
        desc = 'The player will lose only after all units of the player will be killed',
      },
      {
        key  = '1',
        name = 'Commander ends',
        desc = 'The player will lose when his commander will be dead',
      },
      {
        key  = '2',
        name = 'Commander lineage ends',
        desc = 'This is a stricter form of commander ends\nevery unit will inherit the lineage from the player whom built it\neven if shared, when the commander dies the unit will still die',
      },
      {
        key  = '3',
        name = 'Infinite',
        desc = 'Game will never end',
      },
    },
  },

  {
    key    = 'StartingResources',
    name   = 'Starting Resources',
    desc   = 'Sets storage and amount of resources that players will start with',
    type   = 'section',
  },

  {
    key    = 'StartMetal',
    name   = 'Starting metal',
    desc   = 'Determines amount of metal and metal storage that each player will start with',
    type   = 'number',
    section= 'StartingResources',
    def    = 1000,
    min    = 0,
    max    = 10000,
    step   = 1,  -- quantization is aligned to the def value
                    -- (step <= 0) means that there is no quantization
  },
  {
   key    = 'StartMetal',
   scope  = 'team',
   name   = 'Team Starting metal',
   desc   = 'Determines amount of metal and metal storage this team will start with',
   type   = 'number',
   section= 'StartingResources',
   def    = 1000,
   min    = 0,
   max    = 10000,
   step   = 1,  -- quantization is aligned to the def value
   -- (step <= 0) means that there is no quantization
  },
  {
    key    = 'StartEnergy',
    name   = 'Starting energy',
    desc   = 'Determines amount of energy and energy storage that each player will start with',
    type   = 'number',
    section= 'StartingResources',
    def    = 1000,
    min    = 0,
    max    = 10000,
    step   = 1,  -- quantization is aligned to the def value
                    -- (step <= 0) means that there is no quantization
  },
  {
   key    = 'StartEnergy',
   scope  = 'team',
   name   = 'Team Starting energy',
   desc   = 'Determines amount of energy and energy storage that this team will start with',
   type   = 'number',
   section= 'StartingResources',
   def    = 1000,
   min    = 0,
   max    = 10000,
   step   = 1,  -- quantization is aligned to the def value
   -- (step <= 0) means that there is no quantization
  },

  {
    key    = 'MaxUnits',
    name   = 'Max units',
    desc   = 'Maximum number of units (including buildings) for each team allowed at the same time',
    type   = 'number',
    def    = 500,
    min    = 1,
    max    = 10000, --- engine caps at lower limit if more than 3 team are ingame
    step   = 1,  -- quantization is aligned to the def value
                    -- (step <= 0) means that there is no quantization
  },

  {
    key    = 'LimitDgun',
    name   = 'Limit D-Gun range',
    desc   = "The commander's D-Gun weapon will be usable only close to the player's starting location",
    type   = 'bool',
    def    = false,
  },

  {
    key    = 'GhostedBuildings',
    name   = 'Ghosted buildings',
    desc   = "Once an enemy building will be spotted\na ghost trail will be placed to memorize location even after the loss of the line of sight",
    type   = 'bool',
    def    = true,
  },
  {
    key    = 'DiminishingMMs',
    name   = 'Diminishing metal makers efficiency',
    desc   = "Everytime a new metal maker will be built, the energy/metal efficiency ratio will decrease",
    type   = 'bool',
    def    = false,
  },

  {
    key    = 'FixedAllies',
    name   = 'Fixed ingame alliances',
    desc   = 'Disables the possibility of players to dynamically change alliances ingame',
    type   = 'bool',
    def    = false,
  },

  {
    key    = 'LimitSpeed',
    name   = 'Speed Restriction',
    desc   = 'Limits maximum and minimum speed that the players will be allowed to change to',
    type   = 'section',
  },

  {
    key    = 'MaxSpeed',
    name   = 'Maximum game speed',
    desc   = 'Sets the maximum speed that the players will be allowed to change to',
    type   = 'number',
    section= 'LimitSpeed',
    def    = 3,
    min    = 0.1,
    max    = 100,
    step   = 0.1,  -- quantization is aligned to the def value
                    -- (step <= 0) means that there is no quantization
  },

  {
    key    = 'MinSpeed',
    name   = 'Minimum game speed',
    desc   = 'Sets the minimum speed that the players will be allowed to change to',
    type   = 'number',
    section= 'LimitSpeed',
    def    = 0.3,
    min    = 0.1,
    max    = 100,
    step   = 0.1,  -- quantization is aligned to the def value
                    -- (step <= 0) means that there is no quantization
  },

  {
    key    = 'DisableMapDamage',
    name   = 'Undeformable map',
    desc   = 'Prevents the map shape from being changed by weapons',
    type   = 'bool',
    def    = false,
  },
--[[
-- the following options can create problems and were never used by interface programs, thus are commented out for the moment

  {
    key    = 'LuaGaia',
    name   = 'Enables gaia',
    desc   = 'Enables gaia player',
    type   = 'bool',
    def    = true,
  },

  {
    key    = 'NoHelperAIs',
    name   = 'Disable helper AIs',
    desc   = 'Disables luaui and group ai usage for all players',
    type   = 'bool',
    def    = false,
  },

  {
    key    = 'LuaRules',
    name   = 'Enable LuaRules',
    desc   = 'Enable mod usage of LuaRules',
    type   = 'bool',
    def    = true,
  },

-- If you add this to your mods EngineOptions.lua then hosts can set the startscript
-- Remove any scripts that don't work with your mod
  {
    key    = 'ScriptName',
    name   = 'Start Script',
    desc   = 'Changes game behaviour and/or starting conditions',
    type   = 'list',
    def    = 'Commanders',
    items  = {
      {
        key  = 'Air combat test',
        name = 'Air Combat Test',
        desc = 'Loads some planes on start.',
      },
      {
        key  = 'Commanders',
        name = 'Commanders',
        desc = 'Each player gets a command unit.',
      },
      {
        key  = 'Cmds 1000 res',
        name = 'Commanders + 1000 res',
        desc = 'Each player gets a commander and 1000 units of metal and energy.',
      },
      {
        key  = 'EmptyScript',
        name = 'Empty Script',
        desc = 'Do nothing special on game start.',
      },
      {
        key  = 'Random enemies',
        name = 'Random Enemies',
        desc = 'Spawns random enemies',
      },
      {
        key  = 'Random enemies 2',
        name = 'Random Enemies (Autonomous)',
        desc = 'Spawns random enemies',
      },
    },
  },
--]]

}
return options
