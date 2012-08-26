function widget:GetInfo()
return {
	name    = "Test-Widget",
	desc    = "Sets Speed + tries to keep framerate + autoexit",
	author  = "abma",
	date    = "Jul. 2011",
	license = "GNU GPL, v2 or later",
	layer   = 0,
	enabled = true,
}
end

local maxframes = 27000 -- run spring 15 minutes (ingame time)
local initialspeed = 120 -- speed at the beginning

function widget:GameFrame(n)
	if n==0 then -- set gamespeed at start of game
		Spring.SendCommands("setmaxspeed " .. 1000,
			"setminspeed " .. initialspeed,
			"setminspeed 1")
	end
	if n==maxframes then
		Spring.Echo("Tests run long enough, quitting...")
		Spring.SendCommands("quit")
	end
end

