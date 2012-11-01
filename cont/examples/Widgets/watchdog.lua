function widget:GetInfo()
return {
	name    = "Watchdog",
	desc    = "Watches for endless loops in lua-scripts, use only for debugging!",
	author  = "abma",
	date    = "Oct. 2011",
	license = "GNU GPL, v2 or later",
	layer   = 0,
	enabled = true,
}
end

local i=0
local function hook(event)
	i = i + 1
	if ((i % (10^7)) < 1) then
		Spring.Echo("[Watchdog] Hang detected:")
		i = 0
		Spring.Echo(Spring.GetGameFrame(), event, debug.getinfo(2).name)
		Spring.Echo(debug.traceback())
	end
end

function widget:Initialize()
	--enable hook function
	debug.sethook(hook,"l",10^100)
end

function widget:Shutdown()
	--remove hook
	debug.sethook()
end

function widget:Update()
	-- reset watchdog counter
	i=0
end

