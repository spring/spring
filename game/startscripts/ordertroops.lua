class 'MissionTest' (Script)

-- Pass the script name to be displayed to the parent constructor
function MissionTest:__init() super('LUA requestattackers')
    self.state = 0
    
    -- Set to true to not have to press enter at start
    self.onlySinglePlayer = true
    
    self.commands = {}
    self.commands[".help"] = self.Help
    self.commands[".send"] = self.SendTroops
end

function MissionTest:Help(param)
        print("Request troops to attack you by saying for example .send 5 armstump")
end

function MissionTest:SendTroops(param)
    local varpos = string.find(param, " ")
    if varpos then
        number = string.sub(param, 1, varpos - 1)
        trooptype = string.sub(param, varpos + 1)
    end

     print( "number [" .. number .. "]" )
     print( "type [" .. trooptype  .. "]" )
    local c = Command()
    c.id = Command.PATROL
    c:AddParam(self.start0.x)
    c:AddParam(self.start0.y)
    c:AddParam(self.start0.z)
    
    print( "-- cut here -- " )
    print( "if gs.frameNum / 30 == " .. gs.frameNum / 30 .. " then" )
     for i = 1, number do
        self.thisunit = units.Load(trooptype, float3( self.start1.x, self.start1.y, self.start1.z + i * 50 ), 1, false)
        self.thisunit:GiveCommand(c)
        print( "units.Load(" .. trooptype .. ", float3( " .. self.start1.x .. "," .. self.start1.y .. "," .. self.start1.z + i * 50 .. " ), 1, false)" )
    end
    print( "end" )
    print( "-- cut here -- " )
    
end

function MissionTest:Update()

    -- Perform initialization
    if self.state == 0 then
        self.state = 1
        self:Setup()
        
        print("Request troops to attack you by saying for example .send 5 armstump")
    end
    
    -- Run SlowUpdate once every second
    if math.mod(gs.frameNum, 30) == 0 then
        self:SlowUpdate()
    end
end

-- Spring does not call this directly
function MissionTest:SlowUpdate()

end

function MissionTest:GotChatMsg(msg, player)
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


-- Create a bunch of units for this simple mission
function MissionTest:Setup()
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
    
    self.start0 = float3( x0, 80, z0 )
    self.start1 = float3( x1, 80, z1 )
    
    self.com0 = units.Load(s0, self.start0, 0, false)
    --self.factory1 = units.Load( "CORHLT", float3( x1 - 100, 80, z1 ), 1, false )
    self.com1 = units.Load(s1, self.start1, 1, false)
end

-- Instantiate the class so that it is registered and shown
missionTest = MissionTest()
