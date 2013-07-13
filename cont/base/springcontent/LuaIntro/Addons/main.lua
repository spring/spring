
if addon.InGetInfo then
	return {
		name    = "Main",
		desc    = "(music and other stuff)",
		author  = "jK",
		date    = "2012",
		license = "GPL2",
		layer   = 0,
		depend  = {"LoadProgress"},
		enabled = true,
	}
end

------------------------------------------

require "savetable.lua"

local lastLoadMessage = ""

function addon.LoadProgress(message, replaceLastLine)
	lastLoadMessage = message
end

------------------------------------------

Spring.SetSoundStreamVolume(0)
local musicfiles = VFS.DirList(LUA_DIRNAME .. "Assets/music", "*.ogg")
if (#musicfiles > 0) then
	Spring.PlaySoundStream(musicfiles[ math.random(#musicfiles) ], 1)
	Spring.SetSoundStreamVolume(0)
end


local font = gl.LoadFont("FreeSansBold.otf", 50, 20, 1.95)

function addon.DrawLoadScreen()
	local loadProgress = SG.GetLoadProgress()

	-- fade in & out music with progress
	if (loadProgress < 0.9) then
		Spring.SetSoundStreamVolume(loadProgress)
	else
		Spring.SetSoundStreamVolume(0.9 + ((0.9 - loadProgress) * 9))
	end

	local vsx, vsy = gl.GetViewSizes()

	-- draw progressbar
	local hbw = 3.5/vsx
	local vbw = 3.5/vsy
	local hsw = 0.2
	local vsw = 0.2
	gl.BeginEnd(GL.QUADS, function()
		--shadow topleft
		gl.Color(0,0,0,0)
			gl.Vertex(0.2-hsw, 0.2    )
			gl.Vertex(0.2-hsw, 0.2+vsw)
			gl.Vertex(0.2    , 0.2+vsw)
		gl.Color(0,0,0,0.5)
			gl.Vertex(0.2    , 0.2)

		--shadow top
		gl.Color(0,0,0,0)
			gl.Vertex(0.2, 0.2+vsw)
			gl.Vertex(0.8, 0.2+vsw)
		gl.Color(0,0,0,0.5)
			gl.Vertex(0.8, 0.2)
			gl.Vertex(0.2, 0.2)

		--shadow topright
		gl.Color(0,0,0,0)
			gl.Vertex(0.8    , 0.2+vsw)
			gl.Vertex(0.8+hsw, 0.2+vsw)
			gl.Vertex(0.8+hsw, 0.2)
		gl.Color(0,0,0,0.5)
			gl.Vertex(0.8    , 0.2)

		--shadow right
		gl.Color(0,0,0,0)
			gl.Vertex(0.8+hsw, 0.2)
			gl.Vertex(0.8+hsw, 0.15)
		gl.Color(0,0,0,0.5)
			gl.Vertex(0.8    , 0.15)
			gl.Vertex(0.8    , 0.2)

		--shadow btmright
		gl.Color(0,0,0,0)
			gl.Vertex(0.8    , 0.15-vsw)
			gl.Vertex(0.8+hsw, 0.15-vsw)
			gl.Vertex(0.8+hsw, 0.15)
		gl.Color(0,0,0,0.5)
			gl.Vertex(0.8    , 0.15)

		--shadow btm
		gl.Color(0,0,0,0)
			gl.Vertex(0.2, 0.15-vsw)
			gl.Vertex(0.8, 0.15-vsw)
		gl.Color(0,0,0,0.5)
			gl.Vertex(0.8, 0.15)
			gl.Vertex(0.2, 0.15)

		--shadow btmleft
		gl.Color(0,0,0,0)
			gl.Vertex(0.2-hsw, 0.15    )
			gl.Vertex(0.2-hsw, 0.15-vsw)
			gl.Vertex(0.2    , 0.15-vsw)
		gl.Color(0,0,0,0.5)
			gl.Vertex(0.2    , 0.15)

		--shadow left
		gl.Color(0,0,0,0)
			gl.Vertex(0.2-hsw, 0.2)
			gl.Vertex(0.2-hsw, 0.15)
		gl.Color(0,0,0,0.5)
			gl.Vertex(0.2    , 0.15)
			gl.Vertex(0.2    , 0.2)

		--bar bg
		gl.Color(0,0,0,0.85)
			gl.Vertex(0.2, 0.2)
			gl.Vertex(0.8, 0.2)
			gl.Vertex(0.8, 0.15)
			gl.Vertex(0.2, 0.15)

		--progress
		gl.Color(1,1,1,0.7)
			gl.Vertex(0.2, 0.2)
			gl.Vertex(0.2 + math.max(0, loadProgress-0.01) * 0.6, 0.2)
			gl.Vertex(0.2 + math.max(0, loadProgress-0.01) * 0.6, 0.15)
			gl.Vertex(0.2, 0.15)
		gl.Color(1,1,1,0.7)
			gl.Vertex(0.2 + math.max(0, loadProgress-0.01) * 0.6, 0.2)
			gl.Vertex(0.2 + math.max(0, loadProgress-0.01) * 0.6, 0.15)
		gl.Color(1,1,1,0)
			gl.Vertex(0.2 + math.min(1, math.max(0, loadProgress+0.01)) * 0.6, 0.15)
			gl.Vertex(0.2 + math.min(1, math.max(0, loadProgress+0.01)) * 0.6, 0.2)

		--bar borders
		gl.Color(1,1,1,1)
			gl.Vertex(0.2, 0.2)
			gl.Vertex(0.8, 0.2)
			gl.Vertex(0.8, 0.2-vbw)
			gl.Vertex(0.2, 0.2-vbw)
		gl.Color(1,1,1,1)
			gl.Vertex(0.2, 0.15)
			gl.Vertex(0.8, 0.15)
			gl.Vertex(0.8, 0.15+vbw)
			gl.Vertex(0.2, 0.15+vbw)
		gl.Color(1,1,1,1)
			gl.Vertex(0.2, 0.2)
			gl.Vertex(0.2, 0.15)
			gl.Vertex(0.2+hbw, 0.15)
			gl.Vertex(0.2+hbw, 0.2)
		gl.Color(1,1,1,1)
			gl.Vertex(0.8, 0.2)
			gl.Vertex(0.8, 0.15)
			gl.Vertex(0.8-hbw, 0.15)
			gl.Vertex(0.8-hbw, 0.2)
	end)

--[[
	gl.Color(0,0,0,1)
	gl.Rect(0.2,0.15,0.8,0.2)
	gl.Color(1,1,1,1)
	gl.Rect(0.2,0.15,0.2 + math.max(0, loadProgress) * 0.6,0.2)
	gl.LineWidth(5)
	gl.PolygonMode(GL.FRONT_AND_BACK, GL.LINE)
	gl.Rect(0.2,0.15,0.8,0.2)
	gl.PolygonMode(GL.FRONT_AND_BACK, GL.FILL)
	gl.LineWidth(1)
	gl.Color(1,1,1,1)
--]]

	-- progressbar text
	gl.PushMatrix()
	gl.Scale(1/vsx,1/vsy,1)
		local barTextSize = vsy * (0.05 - 0.015)

		--font:Print(lastLoadMessage, vsx * 0.5, vsy * 0.3, 50, "sc")
		font:Print(Game.gameName, vsx * 0.5, vsy * 0.95, vsy * 0.07, "sca")
		font:Print(lastLoadMessage, vsx * 0.2, vsy * 0.14, barTextSize * 0.5, "sa")
		if loadProgress>0 then
			font:Print(("%.0f%%"):format(loadProgress * 100), vsx * 0.5, vsy * 0.165, barTextSize, "oc")
		else
			font:Print("???", vsx * 0.5, vsy * 0.165, barTextSize, "oc")
		end
	gl.PopMatrix()
end


function addon.MousePress(...)
	--Spring.Echo(...)
end


function addon.Shutdown()
	Spring.StopSoundStream()
	Spring.SetSoundStreamVolume(1)
	gl.DeleteFont(font)
end
