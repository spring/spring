--
--  Info Definition Table format
--
--
--  These keywords must be lowercase for LuaParser to read them.
--
--  key:      user defined or one of the SKIRMISH_AI_PROPERTY_* defines in
--            SSkirmishAILibrary.h
--  value:    the value of the property
--  desc:     the description (could be used as a tooltip)
--
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local infos = {
	{
		key    = 'shortName',
		value  = 'AAI',
		desc   = 'machine conform name.',
	},
	{
		key    = 'version',
		value  = '0.9', -- AI version - !This comment is used for parsing!
	},
	{
		key    = 'name',
		value  = 'submarines Skirmish AI (AAI)',
		desc   = 'human readable name.',
	},
	{
		key    = 'description',
		value  = 'Supports most TA based Mods plus a few others. Uses config files.',
		desc   = 'tooltip.',
	},
	{
		key    = 'url',
		value  = 'http://spring.clan-sy.com/wiki/AI:AAI',
		desc   = 'URL with more detailed info about the AI',
	},
	{
		key    = 'loadSupported',
		value  = 'no',
		desc   = 'whether this AI supports loading or not',
	},
	{
		key    = 'interfaceShortName',
		value  = 'C', -- AI Interface name - !This comment is used for parsing!
		desc   = 'the shortName of the AI interface this AI needs',
	},
	{
		key    = 'interfaceVersion',
		value  = '0.1', -- AI Interface version - !This comment is used for parsing!
		desc   = 'the minimum version of the AI interface this AI needs',
	},
}

return infos

