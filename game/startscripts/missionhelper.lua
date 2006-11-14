
-- Create a subclass of the native spring class Script
class 'MissionHelper' (Script)

-- Pass the script name to be displayed to the parent constructor
function MissionHelper:__init() super('LUA missionbuilder2')
    self.state = 0
    self.team = 0
    
    -- Set to true to not have to press enter at start
    self.onlySinglePlayer = true

    -- Define commands that this script handles
    self.commands = {}
    self.commands[".help"] = self.Help
    self.commands[".savesel"] = self.SaveSelected
    self.commands[".savelist"] = self.SaveList
    self.commands[".savefeatures"] = self.SaveFeatures
end

-- This function is executed every simulated frame (30 times/sec)
function MissionHelper:Update()

    -- Perform initialization
    if self.state == 0 then
        self.state = 1
        self:Setup()
        
        print("This script can help in constructing Lua missions")
        print("Type .help for more information")
    end
    
    -- Run SlowUpdate once every second
    if math.mod(gs.frameNum, 15) == 0 then
        self:SlowUpdate()
    end    
end

function MissionHelper:GotChatMsg(msg, player)
    local varpos = string.find(msg, " ")
    local param = false
    if varpos then
        param = string.sub(msg, varpos + 1)
        msg = string.sub(msg, 1, varpos - 1)
    end
                         
    local cmd = self.commands[msg]
    if cmd then
        cmd(self, param)
    end
end

function MissionHelper:Help(param)
    if param and string.find(param, "list") then
        print("This command formats the commands so that the unit " ..
              "references are stored in a list.")
    end
    if param and string.find(param, "save") then
        print("Prints the lua commands for creating the selected " ..
              "units. Copy the contents of infolog.txt to use this.")
        print("Specify an optional team number as second parameter, " ..
              "the created units will belong to that team.")
    else
        print("These commands are available:")
        print(".help .savesel [team] .savelist [team] .savefeatures")
        print("Type .help <command> for more info")
    end
end

function MissionHelper:SaveSelected(param)
    self:Save(param, false)
end

function MissionHelper:SaveList(param)
    self:Save(param, true)
end

function MissionHelper:SaveFeatures(param)
    print("--- cut here ---")
    local list = features.GetAt( float3( 0, 0, 0 ), 10000 )
    prefix = ""
    suffix = ""
    for i = 1, table.getn(list) do
       local def = list[i].definition
       if string.sub(def.name, 1, 4 ) ~= "Tree" then
           local pos = string.format("%.1f, %.1f, %.1f", list[i].pos.x,
                                      list[i].pos.y, list[i].pos.z)
            if tab and i == table.getn(list) then
                suffix = ""
            end
    
            print(prefix .. "features.Load(\"" .. def.name .. "\", float3(" ..
                  pos .. "), 1 )" .. suffix)
        end
    end
    print("--- cut here ---")
end

function MissionHelper:Save(param, tab)
    local list = units.GetSelected(self.team)
    if table.getn(list) < 1 then
        return
    end

    local tid = 1
    if param then
        tid = param
    end

    print("--- cut here ---")
    local prefix = ""
    local suffix = ""
    if tab then
        print("local unitList = {")
        prefix = "    "
        suffix = ","
    end

    local i 
    for i = 1, table.getn(list) do
        local def = list[i].definition
        local pos = string.format("%.1f, %.1f, %.1f", list[i].pos.x,
                                  list[i].pos.y, list[i].pos.z)
        if tab and i == table.getn(list) then
            suffix = ""
        end

        print(prefix .. "units.Load(\"" .. def.name .. "\", float3(" ..
              pos .. "), " .. tid .. ", false)" .. suffix)

    end
    if tab then
        print("}")
    end

    print("--- cut here ---")

end

-- Spring does not call this directly
function MissionHelper:SlowUpdate()
    if self.state == 1 then

        -- We can not read unsynced selection info in a synced script
        -- so we have to make sure it is synced often
        units.SendSelection()
    end
end

-- Spawns a correct commander from team 0 at startpos 0
function MissionHelper:Setup()
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
missionHelper = MissionHelper()
