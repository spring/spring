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
	VFS.DownloadArchive("ba:stable", "game")
end

function widget:DownloadStarted(id)
	Spring.Echo("download finished" .. id)
end

function widget:DownloadQueued(id)
	Spring.Echo("download finished" .. id)
end

function widget:DownloadFinished(id)
	Spring.Echo("download finished" .. id)
end

function widget:DownloadFailed(id, errorid)
	Spring.Echo("download failed" .. id .. errorid)
end

function widget:DownloadProgress(id, downloaded, total)
	Spring.Echo("download progress" .. id)
end

