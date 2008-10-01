
-- Create a subclass of the native spring class Script
class 'MissionTest' (Script)

-- Pass the script name to be displayed to the parent constructor
function MissionTest:__init() super('LUA aistartscripttest')
    self.state = 0

    -- Set to true to not have to press enter at start
    self.onlySinglePlayer = false
end

-- Return the mapname this script wants
function MissionTest:GetMapName()
    return "SmallDivide.smf"
end

-- Return the modname this script wants
function MissionTest:GetModName()
    return "XTA_Installer_Version.sdz"
end

-- This function is executed every simulated frame (30 times/sec)
function MissionTest:Update()

    -- Perform initialization
    if self.state == 0 then
        self.state = 1
        self:Setup()

        print("A small ARM force has been trapped all alone. You must get " ..
              "them to the alien beacon quickly to call in reinforcements!")
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
function MissionTest:SlowUpdate()

    if self.state == 1 then

        local num = units.GetNumAt(self.gatepos, 50)
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
        if not self.cmd.transporter then
            print("The commander is here. Now destroy the CORE forces on " ..
                  "the other side of the pass.")
            self.state = 3
        end
    end

end

-- Create a bunch of units for this simple mission
function MissionTest:Setup()
    AIs.CreateGlobalAI( 0, "AI/Skirmish/impls/TestGlobalAI.dll" )
    AIs.CreateGlobalAI( 1, "AI/Skirmish/impls/TestGlobalAI.dll" )

    units.Load("ARM_Jeffy", float3(300, 80, 300), 0, false)
    units.Load("ARM_Jeffy", float3(340, 80, 300), 0, false)
    units.Load("ARM_Jeffy", float3(380, 80, 300), 0, false)
    units.Load("ARM_Jeffy", float3(420, 80, 300), 0, false)

    units.Load("ARM_Jeffy", float3(1200, 80, 200), 1, false)
    units.Load("ARM_Jeffy", float3(1200, 80, 240), 1, false)
    units.Load("ARM_Jeffy", float3(1200, 80, 280), 1, false)
    units.Load("ARM_Jeffy", float3(1200, 80, 320), 1, false)

    units.Load("ARM_maverick", float3(300, 80, 300), 0, false)
    units.Load("ARM_ZEUS", float3(460, 80, 300), 0, false)

    self.gatepos = float3(2832, 80, 776)
    -- I'm using the targeting facility as a placeholder for the Arm_Beacon (which isn't included in 0.74b3 version of XTA)
    self.gate = units.Load("ARM_targeting_facility", self.gatepos, 0, false)

    self.cmd = units.Load("ARM_commander", float3(3800, 80, 1100), 1, false)
    self.atlas = units.Load("ARM_ATLAS", float3(3800, 80, 1200), 1, false)

    local c = Command()
    c.id = Command.LOAD_UNITS
    c:AddParam(self.cmd.id)
    self.atlas:GiveCommand(c)

    -- Enemy stuff

    local first = {
        units.Load("CORe_AK", float3(1200, 80, 200), 1, false),
        units.Load("CORe_AK", float3(1200, 80, 300), 1, false),
        units.Load("CORe_AK", float3(1200, 80, 400), 1, false),
        units.Load("CORe_AK", float3(1200, 80, 500), 1, false),
        units.Load("CORe_AK", float3(1000, 80, 200), 1, false),
        units.Load("CORe_AK", float3(1000, 80, 300), 1, false),
        units.Load("CORe_AK", float3(1000, 80, 400), 1, false),
        units.Load("CORe_AK", float3(1000, 80, 500), 1, false)
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

    units.Load("core_gaat_gun", float3(1759, 80, 2677), 1, false)
    units.Load("core_gaat_gun", float3(2413, 80, 2626), 1, false)
    units.Load("core_doomsday_machine", float3(2065, 80, 2609), 1, false)
    units.Load("core_cobra", float3(1623, 80, 2967), 1, false)

    units.Load("CORe_FUSion_power_plant", float3(1174, 80, 3271), 1, false)
    units.Load("core_doomsday_machine", float3(974, 80, 3271), 1, false)
    units.Load("core_doomsday_machine", float3(1374, 80, 3271), 1, false)
end

-- Move the commander out of the way
function MissionTest:SetupDelayed()
    local c = Command()
    c.id = Command.MOVE
    c:AddParam(6000)
    c:AddParam(80)
    c:AddParam(1100)
    self.atlas:GiveCommand(c)
end

-- Instantiate the class so that it is registered and shown
missionTest = MissionTest()
