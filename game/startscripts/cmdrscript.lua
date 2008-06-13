
-- Create a subclass of the native spring class Script
class 'CmdrScript' (Script)

-- Pass the script name to be displayed to the parent constructor
function CmdrScript:__init() super('LUA commanderscript')
end

-- Return the mapname this script wants
function CmdrScript:GetMapName()
    return ""
end

-- This function is executed every simulated frame (30 times/sec)
function CmdrScript:Update()
	-- do stuff here
end

-- Spawns a correct commander from team 0 at startpos 0
function CmdrScript:GameStart()
    local p = TdfParser()
    p:LoadFile("gamedata\\sidedata.tdf")

    local s0 = p:GetString("armcom", "side0\\commander");
    local s1 = p:GetString("corcom", "side1\\commander");

    p = TdfParser()
    p:LoadFile(map.GetTDFName())

    local x0, z0, x1, z1
    x0 = p:GetFloat(1000, "MAP\\TEAM0\\StartPosX")
    z0 = p:GetFloat(1000, "MAP\\TEAM0\\StartPosZ")
    x1 = p:GetFloat(1200, "MAP\\TEAM1\\StartPosX")
    z1 = p:GetFloat(1200, "MAP\\TEAM1\\StartPosZ")

    units.Load(s0, float3(x0, 80, z0), 0, false)
    units.Load(s1, float3(x1, 80, z1), 1, false)
end

-- Instantiate the class so that it is registered and shown
cmdrScript = CmdrScript()
