--
--  Info Definition Table format
--
--
--  These keywords must be lowercase for LuaParser to read them.
--
--  key:      user defined or one of the SKIRMISH_AI_PROPERTY_* defines in
--            SSAILibrary.h
--  value:    the value of the property
--  desc:     the description (could be used as a tooltip)
--
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local infos = {
	{
		key    = 'fileName',
		value  = 'NullAI-0.1',
		desc   = 'base of the library file of this AI. "RAI-0.553" means "AI/Skirmish/lib/libRAI-0.553.so", "AI/Skirmish/lib/libRAI-0.553.dylib" or "AI/Skirmish/lib/RAI-0.553.dll"',
	},
	{
		key    = 'shortName',
		value  = 'NullAI',
		desc   = 'machine conform name.',
	},
	{
		key    = 'version',
		value  = '0.1',
	},
	{
		key    = 'name',
		value  = 'Test AI using the new C interface',
		desc   = 'human readable name.',
	},
	{
		key    = 'description',
		value  = 'This AI does absolutely nothing.',
		desc   = 'tooltip.',
	},
	{
		key    = 'url',
		value  = 'http://spring.clan-sy.com/wiki/AI:NullAI',
		desc   = 'URL with more detailed info about the AI',
	},
	{
		key    = 'loadSupported',
		value  = 'no',
		desc   = 'whether this AI supports loading or not',
	},
	{
		key    = 'interfaceShortName',
		value  = 'C',
		desc   = 'the shortName of the AI interface this AI needs',
	},
	{
		key    = 'interfaceVersion',
		value  = '0.1',
		desc   = 'the minimum version of the AI interface this AI needs',
	},
}

return infos
