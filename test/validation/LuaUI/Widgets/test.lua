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

local maxframes = 36000 -- run spring 20 minutes (ingame time)
local initialspeed = 120 -- speed at the beginning
local minunits = 10 -- if fewer than this units are created/destroyed print a warning
local timer
local unitscreated = 0
local unitsdestroyed = 0
local maxruntime = 120 -- run at max 2 minutes

local function ShowStats()
	local time = Spring.DiffTimers(Spring.GetTimer(), timer)
	local gameseconds = Spring.GetGameSeconds()
	local speed = gameseconds / time
	Spring.Echo("Test done:")
	Spring.Echo(string.format("Realtime %is gametime: %is", time, gameseconds ))
	Spring.Echo(string.format("Run at %.2fx real time", speed))
	Spring.Echo(string.format("Units created: %i Units destroyed: %i", unitscreated, unitsdestroyed))
	if unitscreated <= minunits or unitsdestroyed <= minunits then
		Spring.Log("test.lua", LOG.ERROR, string.format("Fewer then minunits %i units were created/destroyed!", minunits))
	end
end

function widget:GameOver()
	Spring.Echo("GameOver called!")
	ShowStats()
	Spring.SendCommands("quit")
end

function widget:Update()
	if (Spring.DiffTimers(Spring.GetTimer(), timer)) > maxruntime then
		Spring.Log("test.lua", LOG.WARNING, string.format("Tests run longer than %i seconds, aborting!", maxruntime ))
		Spring.SendCommands("pause 1", "quit")
	end
end

function widget:Initialize()
	timer = Spring.GetTimer()
	Spring.SendCommands("setmaxspeed " .. 1000,
		"setminspeed " .. initialspeed,
		"setminspeed 1")
end

function widget:GameFrame(n)
	if n==maxframes then
		ShowStats()
		Spring.SendCommands("quit")
	end
end


function widget:UnitCreated(unitID, unitDefID, unitTeam)
	unitscreated = unitscreated + 1
end

function widget:UnitDestroyed(unitID, unitDefID, unitTeam)
	unitsdestroyed = unitsdestroyed + 1
end

