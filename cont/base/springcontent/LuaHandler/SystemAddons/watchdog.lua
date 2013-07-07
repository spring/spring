
if addon.InGetInfo then
	return {
		name      = "Watchdog";
		desc      = "";
		version   = 0.1;
		author    = "jK";
		date      = "2013";
		license   = "GNU GPL, v2 or later";

		layer     = math.huge;
		hidden    = not true; -- don't show in the widget selector
		api       = true; -- load before all others?

		enabled   = not true; -- loaded by default?
	}
end


local i=0
local function hook(event)
	i = i + 1
	if ((i % (10^4)) < 1) then
		i = 0
		Spring.Echo(Spring.GetGameFrame(), event, debug.getinfo(2).name)
		Spring.Echo(debug.traceback())
	end
end


function addon.Initialize()
	debug.sethook(hook,"r",10^100)
end


function addon.Shutdown()
	debug.sethook(nil,"r")
end
