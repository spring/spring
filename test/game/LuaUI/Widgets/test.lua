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
local Spring_GetFPS = Spring.GetFPS
local os_clock = os.clock
local lastcheck = math.floor(os_clock())
function widget:GameFrame(n)
	if n==0 then
		Spring.SendCommands("setmaxspeed 300")
		Spring.SendCommands("setminspeed 30") -- set speed at start to 40
		Spring.SendCommands("setminspeed 1")
	end
	local now = math.floor(os_clock()) --try to run it at frame rate >10 and <30 by adjusting game speed
	if (n>100) and (now > lastcheck) then -- adjust fps only once a second
		lastcheck = now
		fps = Spring_GetFPS()
		if (fps<10) then
			Spring.SendCommands("slowdown")
		elseif (fps>30) then
			Spring.SendCommands("speedup")
		end
	end
	if n==27000 then -- run spring 15 minutes (ingame time)
		Spring.Echo("Tests run long enough, quitting...")
		Spring.SendCommands("quit")
	end
end

