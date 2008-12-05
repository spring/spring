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
		key    = 'fileName',
		value  = 'Java-0.1',
		desc   = 'base of the library file of this AI. "C-0.1" means "AI/Interfaces/impls/libC-0.1.so", "AI/Interfaces/impls/libC-0.1.dylib" or "AI/Interfaces/impls/C-0.1.dll"',
	},
	{
		key    = 'shortName',
		value  = 'Java',
		desc   = 'machine conform name.',
	},
	{
		key    = 'version',
		value  = '0.1',
	},
	{
		key    = 'name',
		value  = 'default Java AI Interface',
		desc   = 'human readable name.',
	},
	{
		key    = 'description',
		value  = 'This interface is needed for Java AIs',
		desc   = 'tooltip.',
	},
	{
		key    = 'url',
		value  = 'http://spring.clan-sy.com/wiki/AIInterface:Java',
		desc   = 'URL with more detailed info about the AI',
	},
	{
		key    = 'supportedLanguages',
		value  = 'Java (possily Groovy, JRuby, ...)',
	},
}

return infos
