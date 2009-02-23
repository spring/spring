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
		key    = 'shortName',
		value  = 'NullOOJavaAI',
		desc   = 'machine conform name.',
	},
	{
		key    = 'version',
		value  = '0.1', -- AI version - !This comment is used for parsing!
	},
	{
		key    = 'className',
		value  = 'nulloojavaai.NullOOJavaAIFactory',
		desc   = 'fully qualified name of a class that implements interface com.clan_sy.spring.ai.AI',
	},
	{
		key    = 'name',
		value  = 'low-level Java test Skirmish AI',
		desc   = 'human readable name.',
	},
	{
		key    = 'loadSupported',
		value  = 'no',
		desc   = 'whether this AI supports loading or not',
	},
	{
		key    = 'interfaceShortName',
		value  = 'Java',
		desc   = 'the shortName of the AI interface this AI needs',
	},
	{
		key    = 'interfaceVersion',
		value  = '0.1',
		desc   = 'the minimum version of the AI interface required by this AI',
	},
}

return infos
