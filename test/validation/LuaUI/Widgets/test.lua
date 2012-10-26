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
local timer

local function ShowStats()
	local time = Spring.DiffTimers(Spring.GetTimer(), timer)
	local gameseconds = Spring.GetGameSeconds()
	local speed = gameseconds / time
	Spring.Echo("Test done:")
	Spring.Echo(string.format("Realtime %is gametime: %is", time, gameseconds ))
	Spring.Echo(string.format("Run at %.2fx real time", speed))
end

function widget:GameOver()
	Spring.Echo("GameOver called!")
	ShowStats()
end

function widget:GameFrame(n)
	if n==0 then -- set gamespeed at start of game
		Spring.SendCommands("setmaxspeed " .. 1000,
			"setminspeed " .. initialspeed,
			"setminspeed 1")
		timer = Spring.GetTimer()
	end
	if n==maxframes then
		ShowStats()
		Spring.SendCommands("quit")
	end
end

