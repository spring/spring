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
local minfps = 10 -- slow down gamespeed if below this value
local maxfps = 30 -- increase gamespeed if fps higher as this value
local initialspeed = 30 -- speed at the beginning
local os_clock = os.clock
local lastcheck = math.floor(os_clock())

function widget:GameFrame(n)
	if n==0 then -- set gamespeed at start of game
		Spring.SendCommands("setmaxspeed " .. 1000,
			"setminspeed " .. initialspeed,
			"setminspeed 1")
	end
	local now = math.floor(os_clock()) --try to run it at frame rate >10 and <30 by adjusting game speed
	if n==maxframes then
		Spring.Echo("Tests run long enough, quitting...")
		Spring.SendCommands("quit")
	elseif (n>100) and (now > lastcheck) then -- adjust fps only once a second
		lastcheck = now
		fps = Spring.GetFPS()
		if (fps<minfps) then
			Spring.SendCommands("slowdown")
		elseif (fps>maxfps) then
			Spring.SendCommands("speedup")
		end
	end
end

