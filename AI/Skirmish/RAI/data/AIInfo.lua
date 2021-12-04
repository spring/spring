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
		value  = 'RAI',
		desc   = 'machine conform name.',
	},
	{
		key    = 'version',
		value  = '0.601', -- AI version - !This comment is used for parsing!
	},
	{
		key    = 'name',
		value  = 'Reths Skirmish AI (RAI)',
		desc   = 'human readable name.',
	},
	{
		key    = 'description',
		value  = [[
Plays a nice, slow game, concentrating on base building.
Uses ground, air, hovercrafts and water well.
Does some smart moves and handles T2+ well.
Realtive CPU usage: 1.0
Scales well with growing unit numbers.
Known to Support: BA, SA, XTA, NOTA]],
		desc   = 'this should help noobs to find out whether this AI is what they want',
	},
	{
		key    = 'url',
		value  = 'https://springrts.com/wiki/AI:RAI',
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
