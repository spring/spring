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
		value  = 'KAIK',
		desc   = 'machine conform name.',
	},
	{
		key    = 'version',
		value  = '0.13', -- AI version - !This comment is used for parsing!
	},
	{
		key    = 'name',
		value  = 'Kloots Skirmish AI (KAIK)',
		desc   = 'human readable name.',
	},

	{
		key    = 'description',
		value  = [[
Plays a though and fast game, using nearly exclusively ground units,
with a lonesome plane then and when. Attacks in big groups,
and never with hovercraft or water units.
Usually is easy to beat once the game gets to T2.
Relative CPU usage: 1.5
Scales well with growing unit numbers.
Known to Support: BA, SA, XTA
recommended: use only for *A mods
		]],
		desc   = 'this should help noobs to find out whether this AI is what they want',
	},

	{
		key    = 'url',
		value  = 'http://spring.clan-sy.com/wiki/AI:KAIK',
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

	{
		key    = "supportedMods",
		value  = [[
(BA, Balanced Annihilation, *, *)
(BOTA, Basically OTA, *, *)
(SA, Supreme Annihilation, *, *)
(XTA, XTA, *, *)
(EE, Expand and Exterminate, 0.171, 0.46)
(EvoRTS, Evolution RTS, *, *)
		]],
		desc   = "mods this AI is confirmed to be able to play",
	}
}

return infos
