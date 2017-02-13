--
--  Info Definition Table format
--
--
--  These keywords must be lowercase for LuaParser to read them.
--
--  key:      user defined or one of the AI_INTERFACE_PROPERTY_* defines in
--            SAIInterfaceLibrary.h
--  value:    the value of the property
--  desc:     the description (could be used as a tooltip)
--
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local infos = {
	{
		key    = 'shortName',
		value  = 'C',
		desc   = 'machine conform name.',
	},
	{
		key    = 'version',
		value  = '0.1',
	},
	{
		key    = 'name',
		value  = 'default C AI Interface',
		desc   = 'human readable name.',
	},
	{
		key    = 'description',
		value  = 'This interface is needed for C and C++ AIs',
		desc   = 'tooltip.',
	},
	{
		key    = 'url',
		value  = 'https://springrts.com/wiki/AIInterface:C',
		desc   = 'URL with more detailed info about the AI',
	},
	{
		key    = 'supportedLanguages',
		value  = 'C, C++',
	},
	{
		key    = 'supportsLookup',
		value  = 'false',
	},
}

return infos
