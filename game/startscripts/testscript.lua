
-- Create a subclass of the native spring class Script
class 'TestScript' (Script)

-- Pass the script name to be displayed to the parent constructor
function TestScript:__init() super('LUA missiontest')
	self.state = 0
	
	-- Set to true to not have to press enter at start
	self.onlySinglePlayer = true
end

-- Return the mapname this script wants
function TestScript:GetMapName()
	return "SmallDivide.smf"
end

-- This function is executed every simulated frame (30 times/sec)
function TestScript:Update()

	-- Perform initialization
	if self.state == 0 then
		self:Setup()
		self.state = 1
		
		print("A small ARM force has been trapped all alone. You must get them to the galactic gate quickly to call in reinforcements!")				
	end
	
	-- Run SlowUpdate once every second
	if math.mod(gs.frameNum, 30) == 0 then
		self:SlowUpdate()
	end

	
	-- Do some stuff a bit later
	if gs.frameNum == 30*10 then
		self:SetupDelayed()
	end		
end

-- Spring does not call this directly
function TestScript:SlowUpdate()

	if self.state == 1 then
		local num = GetNumUnitsAt(self.gatepos, 50)
		
		if num > 1 then
			print("Communication received. Reinforcements are on their way.")
		
			self.state = 2
		
			self.cmd:ChangeTeam(0, Unit.GIVEN)
			self.atlas:ChangeTeam(0, Unit.GIVEN)
		
			local c = Command()
			c.id = Command.UNLOAD_UNIT
			c:AddParam(3000)
			c:AddParam(80)
			c:AddParam(800)
			self.atlas:GiveCommand(c)		
		end
	
	elseif self.state == 2 then
	    if not self.cmd.inTransport then
			print("The commander is here. Now destroy the CORE forces on the other side of the pass.")
	   	    self.state = 3
	    end
	end
	
end

-- Create a bunch of units for this simple mission
function TestScript:Setup()
	LoadUnit("ARMMAV", float3(300, 80, 300), 0, false)
	LoadUnit("ARMZEUS", float3(400, 80, 300), 0, false)
	
	self.gatepos = float3(2732, 80, 776)
	self.gate = LoadUnit("CORGATE", self.gatepos, 0, false)
	
	self.cmd = LoadUnit("ARMCOM", float3(3800, 80, 1100), 1, false)
	self.atlas = LoadUnit("ARMATLAS", float3(3800, 80, 1200), 1, false)
	
	local c = Command()
	c.id = Command.LOAD_UNITS
	c:AddParam(self.cmd.id)
	self.atlas:GiveCommand(c) 	
								
	-- Enemy stuff
			
	local first = {
	    LoadUnit("CORAK", float3(1200, 80, 200), 1, false),
		LoadUnit("CORAK", float3(1200, 80, 300), 1, false),
		LoadUnit("CORAK", float3(1200, 80, 400), 1, false),
		LoadUnit("CORAK", float3(1200, 80, 500), 1, false),
	    LoadUnit("CORAK", float3(1000, 80, 200), 1, false),
		LoadUnit("CORAK", float3(1000, 80, 300), 1, false),
		LoadUnit("CORAK", float3(1000, 80, 400), 1, false),
		LoadUnit("CORAK", float3(1000, 80, 500), 1, false)	    		
		
	}
	
	local c = Command()
	c.id = Command.PATROL
	c:AddParam(1100)
	c:AddParam(80)
	c:AddParam(1000)
	
	for i = 1, table.getn(first) do
		first[i]:GiveCommand(c)
	end 
				
	-- And a pretty base (a bit small perhaps)	
	
	LoadUnit("CORHLT", float3(1759, 80, 2677), 1, false)
	LoadUnit("CORHLT", float3(2413, 80, 2626), 1, false)
	LoadUnit("CORDOOM", float3(2065, 80, 2609), 1, false)				

	LoadUnit("CORFLAK", float3(1623, 80, 2967), 1, false)	

	LoadUnit("CORFUS", float3(1174, 80, 3271), 1, false)
	LoadUnit("CORDOOM", float3(974, 80, 3271), 1, false)				
	LoadUnit("CORDOOM", float3(1374, 80, 3271), 1, false)						
end

-- Move the commander out of the way
function TestScript:SetupDelayed()
	local c = Command()
	c.id = Command.MOVE
	c:AddParam(6000)
	c:AddParam(80)
	c:AddParam(1100)
	self.atlas:GiveCommand(c)
end

-- Spawns a correct commander from team 0 at startpos 0
function TestScript:SetupRegular()
	local p = SunParser()
	p:LoadFile("gamedata\\sidedata.tdf")
	
	local s0 = p:GetString("armcom", "side0\\commander");
	print("side 0 is", s0)
	
	p = SunParser()
	p:LoadFile("maps\\" .. GetMapName() .. ".smd")
	
	local x0, z0
	x0 = p:GetFloat(1000, "MAP\\TEAM0\\StartPosX")
	z0 = p:GetFloat(1000, "MAP\\TEAM0\\StartPosZ")
	
	print("Startpos0 is at ", x0, z0)
	
	self.player = LoadUnit(s0, float3(x0, 80, z0), 0, false)
end

-- Instantiate the class so that it is registered and shown
testScript = TestScript()