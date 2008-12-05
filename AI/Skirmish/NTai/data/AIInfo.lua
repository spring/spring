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
		value  = 'NTai-UNKNOWN_VERSION',
		desc   = 'base of the library file of this AI. "RAI-0.553" means "AI/Skirmish/lib/libRAI-0.553.so", "AI/Skirmish/lib/libRAI-0.553.dylib" or "AI/Skirmish/lib/RAI-0.553.dll"',
	},
	{
		key    = 'shortName',
		value  = 'NTai',
		desc   = 'machine conform name.',
	},
	{
		key    = 'version',
		value  = 'UNKNOWN_VERSION',
	},
	{
		key    = 'name',
		value  = 'AFs Skirmish AI (NTai). Uses config files.',
		desc   = 'human readable name.',
	},
	{
		key    = 'description',
		value  = 'Competetive AI that supports most TA based Mods and plays decently.',
		desc   = 'tooltip.',
	},
	{
		key    = 'url',
		value  = 'http://spring.clan-sy.com/wiki/AI:NTai',
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

