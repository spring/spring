--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    main.lua
--  brief:   the entry point from gui.lua, relays call-ins to the widget manager
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

Spring.SendCommands({"ctrlpanel " .. LUAUI_DIRNAME .. "ctrlpanel.txt"})

dofile(LUAUI_DIRNAME .. "utils.lua")

include("colors.h.lua")
include("opengl.h.lua")
include("spring.h.lua")

include("debug.lua");
include("layout.lua"); -- contains a simple LayoutButtons()
include("widgets.lua");

include("savetable.lua")


--------------------------------------------------------------------------------
--
-- print the header
--

if (RestartCount == nil) then
  RestartCount = 0
else 
  RestartCount = RestartCount + 1
end

do
  local restartStr = ""
  if (RestartCount > 0) then
    restartStr = "  (" .. RestartCount .. " Restarts)"
  end
  Spring.SendCommands({"echo " .. LUAUI_VERSION .. restartStr})
end


--------------------------------------------------------------------------------

widgetHandler:Initialize()

local gl = Spring.Draw  --  easier to use


-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
--
--  A few helper functions
--

function Echo(msg)
  Spring.SendCommands({'echo ' .. msg})
end


function Say(msg)
  Spring.SendCommands({'say ' .. msg})
end


-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
--
--  UpdateLayout  --  called every frame
--

activePage = 0

forceLayout = true


-- FIXME
function ptuc(teamID)
  local tuc = Spring.GetTeamUnitsCounts(teamID)
  tuc.n = nil
  if (tuc.unknown) then
    print('Unknown = ' .. tuc.unknown)
    tuc.unknown = nil
  end
  
  for udid,count in pairs(tuc) do
    print(UnitDefs[udid].name .. ' = ' .. count)
  end
end



function UpdateLayout(cmdsChanged, page, alt, ctrl, meta, shift)

  local needUpdate = forceLayout
  forceLayout = false

  if (activePage ~= page) then
    needUpdate = true
  end
  activePage = page

  widgetHandler:Update()


if (false) then  -- FIXME
    local mx, my = Spring.GetMouseState()
    local type,data = Spring.TraceScreenRay(mx, my)
    if (type == 'unit') then
      local udid = Spring.GetUnitDefID(data)
      Spring.SendCommands({'echo unit('..data..') '..udid..' '..UnitDefs[udid].name})
    end
end



if (false) then   -- FIXME
  if (Spring.GetGameSeconds() > 0) then
    local mx, my = Spring.GetMouseState()
    local type,pos = Spring.TraceScreenRay(mx, my)
    local id = pos
    if (type == 'ground') then
      print('ground', pos[1], pos[2], pos[3])
      print('ground', pos[1], Spring.GetGroundHeight(pos[1], pos[3]), pos[3])
      print('normal', Spring.GetGroundNormal(pos[1], pos[3]))
      local n,m,h,ts,ks,hs,ss = Spring.GetGroundInfo(pos[1], pos[3])
      print(n,m,h,ts,ks,hs,ss)
    elseif (type == 'unit') then
      print('unit('..id..')', Spring.GetUnitPosition(id))
    elseif (type == 'feature') then
      local x,y,z = Spring.GetFeaturePosition(id)
      print('feature('..id..')', x,y,z,
                                 Spring.GetFeatureResources(id))
      print('  ', Spring.GetFeatureInfo(id))
    end    
  end
end
  
  return needUpdate
end


-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
--
--  DrawScreenItems
--

function DrawScreenItems(windowSizeX, windowSizeY)
  widgetHandler:SetViewSize(windowSizeX, windowSizeY)
  widgetHandler:DrawScreenItems()
end


-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
--
--  WidgetHandler calls
--

function Shutdown()
  return widgetHandler:Shutdown()
end

function ConfigureLayout(command)
  return widgetHandler:ConfigureLayout(command)
end

function CommandNotify(id, params, options)
  return widgetHandler:CommandNotify(id, params, options)
end

function DrawWorldItems()
  return widgetHandler:DrawWorldItems()
end

function DrawScreenItems(vsx, vsy)
  widgetHandler:SetViewSize(vsx, vsy)
  return widgetHandler:DrawScreenItems()
end

function KeyPress(key, mods, isRepeat)
  return widgetHandler:KeyPress(key, mods, isRepeat)
end

function KeyRelease(key)
  return widgetHandler:KeyRelease(key)
end

function MouseMove(x, y, dx, dy, button)
  return widgetHandler:MouseMove(x, y, dx, dy, button)
end

function MousePress(x, y, button)
  return widgetHandler:MousePress(x, y, button)
end

function MouseRelease(x, y, button)
  return widgetHandler:MouseRelease(x, y, button)
end

function IsAbove(x, y)
  return widgetHandler:IsAbove(x, y)
end

function GetTooltip(x, y)
  return widgetHandler:GetTooltip(x, y)
end

function AddConsoleLine(msg, priority)
  return widgetHandler:AddConsoleLine(msg, priority)
end

function GroupChanged(groupID)
  return widgetHandler:GroupChanged(groupID)
end

function UnitCreated(unitID, unitDefID, unitTeam)
  return widgetHandler:UnitCreated(unitID, unitDefID, unitTeam)
end

function UnitFinished(unitID, unitDefID, unitTeam)
  return widgetHandler:UnitFinished(unitID, unitDefID, unitTeam)
end

function UnitFromFactory(unitID, unitDefID, unitTeam,
                         factID, factDefID, userOrders)
  return widgetHandler:UnitFromFactory(unitID, unitDefID, unitTeam,
                                       factID, factDefID, userOrders)
end

function UnitDestroyed(unitID, unitDefID, unitTeam)
  return widgetHandler:UnitDestroyed(unitID, unitDefID, unitTeam)
end

function UnitTaken(unitID, unitDefID, unitTeam, newTeam)
  return widgetHandler:UnitTaken(unitID, unitDefID, unitTeam, newTeam)
end


function UnitGiven(unitID, unitDefID, unitTeam, oldTeam)
  return widgetHandler:UnitGiven(unitID, unitDefID, unitTeam, oldTeam)
end


--------------------------------------------------------------------------------

