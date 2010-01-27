--
-- Example AIOptions.lua
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Custom Options Definition Table format
--
-- NOTES:
-- - using an enumerated table lets you specify the options order
--
-- These keywords must be lowercase for LuaParser to read them.
--
-- key:      the string used in the script.txt
-- name:     the displayed name
-- desc:     the description (could be used as a tooltip)
-- type:     the option type (bool|number|string|list|section)
-- section:  can be used to group this option with others (optional)
-- def:      the default value
-- min:      minimum value for type=number options
-- max:      maximum value for type=number options
-- step:     quantization step, aligned to the def value
-- maxlen:   the maximum string length for type=string options
-- items:    array of item strings for type=list options
-- scope:    'all', 'player', 'team', 'allyteam'      <<< not supported yet >>>
--
-- This is an example file, show-casting all the possibilities of this format.
-- It contains one example for option types bool, string and list, and two
-- for number and section.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------


local options = {


	{ -- section
		key    = 'cheats',
		name   = 'Cheats',
		desc   = 'Fine-Grained settings specifying which kind of cheats \nthe AI is allowed to use.',
		type   = 'section',
	},

	{ -- section
		key    = 'performance',
		name   = 'Performance Relevant Settings',
		desc   = 'These settings may be relevant for both CPU usage and AI difficulty.',
		type   = 'section',
	},

	{ -- list
		key     = 'aggressiveness',
		name    = 'Level Of Aggressivity',
		desc    = 'How aggressive the AI should act.\nkey: aggressiveness',
		type    = 'list',
		section = 'performance',
		def     = 'normal',
		items   = {
			{
				key  = 'deffensive',
				name = 'Deffensive',
				desc = 'The AI will hardly ever attack.',
			},
			{
				key  = 'normal',
				name = 'Normal',
				desc = 'The AI will try to keep a balance between defensive and aggressive play styles.',
			},
			{
				key  = 'aggressive',
				name = 'Aggressive',
				desc = 'The AI will put most of its resources into building attack forces.',
			},
		},
	},

	{ -- number (decimal)
		key     = 'maxgroupsize',
		name    = 'Max Group Size',
		desc    = 'Maximum number of units per group, which the AI tries to keep together.\nkey: maxgroupsize',
		type    = 'number',
		section = 'performance',
		def     = 10,
		min     = 2,
		max     = 50,
		step    = 1.0,
	},

	{ -- number (float)
		key     = 'resourcebonous',
		name    = 'Resource Bonous',
		desc    = 'The AI will get 1+[this-value] times as muhc resources as a normal player.\nkey: resourcebonous',
		type    = 'number',
		section = 'cheats',
		def     = 0.0,
		min     = 0.0,
		max     = 20.0,
		-- quantization is aligned to the def value
		-- (step <= 0) -> there is no quantization
		step    = 0.1,
	},

	{ -- bool
		key     = 'maphack',
		name    = 'Map Hack',
		desc    = 'Whether the AI can see everything on the map, always, or is bount to LOS & Radar usage.\nkey: maphack',
		type    = 'bool',
		section = 'cheats',
		def     = false,
	},

	{ -- string
		key     = 'reporturl',
		name    = 'Report URL',
		desc    = 'Statistics and learning data will be sent ot this URL every 5 minutes.\nkey: reporturl',
		type    = 'string',
		def     = 'http://myAIStats.myDomain.com/statsReceiver.cgi',
	},

}

return options

