
if addon.InGetInfo then
	return {
		name    = "LoadProgress",
		desc    = "",
		author  = "jK",
		date    = "2012",
		license = "GPL2",
		layer   = 0,
		enabled = true,
	}
end

------------------------------------------

local checkpoints = {
		"Parsing Map Information",
		"Loading SMF",
		"Loading Radar Icons",
		"Loading GameData Definitions",
		"Loading Sound Definitions",
		"Creating Smooth Height Mesh",
		"Creating QuadField & CEGs",
		"Creating Unit Textures",
		"Creating Sky",
		"Loading Weapon Definitions",
		"Loading Unit Definitions",
		"Loading Feature Definitions",
		"Initializing Map Features",
		"Creating ShadowHandler & DecalHandler",
		"Creating GroundDrawer",
		"Loading Tile Files",
		"Reading Tile Map",
		"Loading Square Textures",
		"Creating TreeDrawer",
		"Creating ProjectileDrawer & UnitDrawer",
		"Creating Projectile Textures",
		"Creating Water",
		"Loading LuaRules",
		"Loading LuaGaia",
		"Loading LuaUI",
		"Initializing PathCache",
		"Finalizing",
	}

local function endswith(s, send)
	return #s >= #send and s:find(send, #s-#send+1, true) and true or false
end

local pos = 0 -- position in checkpoints
local progress = 1 -- loading progress: 0..1

function addon.LoadProgress(str)
	for i = pos + 1,#checkpoints do
		if endswith(str, checkpoints[i]) then -- checkpoint found, update progress
			if i > pos + 1 then
				Spring.Log("loadprogress.lua", LOG.WARNING, "skipped checkpoint" .. checkpoints[i-1])
			end
			-- Spring.Log("loadprogress.lua", LOG.WARNING, "checkpoint " .. checkpoints[i])
			pos = i
			break
		end
	end
	-- update progress for SG.GetLoadProgress()
	progress = pos/#checkpoints
end

function SG.GetLoadProgress()
	return progress
end

