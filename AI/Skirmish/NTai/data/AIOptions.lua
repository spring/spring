--
--  Custom Options Definition Table format
--
--  NOTES:
--  - using an enumerated table lets you specify the options order
--
--  These keywords must be lowercase for LuaParser to read them.
--
--  key:      the string used in the script.txt
--  name:     the displayed name
--  desc:     the description (could be used as a tooltip)
--  type:     the option type
--  def:      the default value;
--  min:      minimum value for number options
--  max:      maximum value for number options
--  step:     quantization step, aligned to the def value
--  maxlen:   the maximum string length for string options
--  items:    array of item strings for list options
--  scope:    'all', 'player', 'team', 'allyteam'      <<< not supported yet >>>
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local options = {
	{
		key="difficulty",
		name="AI Difficulty Level",
		desc="1 means, the AI plays very poor, 5 means, it gives its best",
		type   = 'number',
		def    = 1,
		min    = 1,
		max    = 1,
		step   = 1,
	},
}

return options
