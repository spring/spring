function widget:GetInfo()
return {
	name    = "Downloader test widget",
	desc    = "a simple test widget to show capabilities of downloader",
	author  = "abma",
	date    = "Nov. 2014",
	license = "GNU GPL, v2 or later",
	layer   = 0,
	enabled = true,
}
end

function widget:Initialize()
	Spring.Echo("Starting to download")
	Spring.Download("ba:stable", "game")
end
