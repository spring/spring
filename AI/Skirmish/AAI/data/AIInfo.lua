--[[
--------------------------------------------------------------------------------

	Info Definition Table format

	These keywords must be lowercase for LuaParser to read them.

	key:    user defined or one of the SKIRMISH_AI_PROPERTY_* defines in
		    SSkirmishAILibrary.h
	value:  the value of the property
	desc:   the description (could be used as a tooltip)

--------------------------------------------------------------------------------
]]

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
		value  = [[
Plays a nice and slow game, using ground, air & hovercrafts.
Though water gets used too, it happens seldomly.
Sometimes it uses smart moves, but needs many test games
to get better through aquiring learning data.
Realtive CPU usage: 1.0
Scales well with growing unit numbers.
Special: needs a (manually managed) config file per mod.
Known to Support (if config available): BA, SA, XTA, S44]],
		desc   = 'this should help noobs to find out whether this AI is what they want',
	},
	{
		key    = 'url',
		value  = 'https://springrts.com/wiki/AI:AAI',
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

