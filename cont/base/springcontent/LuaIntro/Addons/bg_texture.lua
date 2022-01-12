
if addon.InGetInfo then
	return {
		name    = "LoadTexture",
		desc    = "",
		author  = "jK",
		date    = "2012",
		license = "GPL2",
		layer   = 2,
		depend  = {"LoadProgress"},
		enabled = true,
	}
end

------------------------------------------

local loadscreens = VFS.DirList("bitmaps/loadpictures/")
local backgroundTexture
if #loadscreens ~= 0 then
	backgroundTexture = loadscreens[ math.random(#loadscreens) ]
end
local aspectRatio


function addon.DrawLoadScreen()
	if backgroundTexture == nil then
		return
	end
	local loadProgress = SG.GetLoadProgress()

	if not aspectRatio then
		local texInfo = gl.TextureInfo(backgroundTexture)
		if not texInfo then return end
		aspectRatio = texInfo.xsize / texInfo.ysize
	end

	local vsx, vsy = gl.GetViewSizes()
	local screenAspectRatio = vsx / vsy

	local xDiv = 0
	local yDiv = 0
	local ratioComp = screenAspectRatio / aspectRatio

	if (ratioComp > 1) then
		xDiv = (1 - (1 / ratioComp)) * 0.5;
	elseif (math.abs(ratioComp - 1) < 0) then
	else
		yDiv = (1 - ratioComp) * 0.5;
	end

	-- background
	--fade in: gl.Color(1,1,1,1 - (1 - loadProgress)^5)
	gl.Color(1,1,1,1)
	gl.Texture(backgroundTexture)
	gl.TexRect(0+xDiv,0+yDiv,1-xDiv,1-yDiv)
	gl.Texture(false)
end

function addon.Shutdown()
	if backgroundTexture == nil then
		return
	end
	gl.DeleteTexture(backgroundTexture)
end
