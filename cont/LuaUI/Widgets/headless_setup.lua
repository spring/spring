if (Spring.GetConfigInt('Headless', 0) == 0) then
   return false
end

function widget:GetInfo()
   return {
      name = "HeadlessSetup",
      desc = "Setup for headless botrunner games.",
      author = "aegis",
      date = "October 18, 2009",
      license = "Public Domain",
      layer = 0,
      enabled = true,
   }
end

local startingSpeed = 120
local timer
local headless

function widget:Initialize()
   headless = (Spring.GetConfigInt('Headless', 0) ~= 0)
   if (headless) then
      Spring.Echo('Prepping for headless...')
      Spring.SendCommands(
         string.format('setmaxspeed %i', startingSpeed),
         string.format('setminspeed %i', startingSpeed),
         'hideinterface'
         )
   end
end

function widget:GameStart()
   Spring.Echo('Game started... starting timer.')
   timer = Spring.GetTimer()
end

function widget:GameOver()
   local time = Spring.DiffTimers(Spring.GetTimer(), timer)
   Spring.Echo(string.format('Game over, realtime: %i seconds, gametime: %i seconds', time, Spring.GetGameSeconds()))
end

