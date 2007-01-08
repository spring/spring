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

-- ???: playing around
do
  for i = 0,9 do
    Spring.SendCommands({
      "unbindaction     group"..i,
      "bind     "..i.." group "..i,
      "bind   C+"..i.." group "..i.." set",
      "bind C+S+"..i.." group "..i.." add",
      "bind   S+"..i.." group "..i.." selectadd",
      "bind   A+"..i.." group "..i.." selecttoggle",
      "bind   M+"..i.." group "..i.." selecttoggle"
    })
  end
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


function UpdateLayout(cmdsChanged, page, alt, ctrl, meta, shift)
  local needUpdate = false

--  needUpdate = needUpdate or (activePage ~= page)
--  activePage = page
  
  if (cmdsChanged) then
    widgetHandler:CommandsChanged()
  end

  local lastSecs = Spring.GetLastFrameSeconds()  
  needUpdate = needUpdate or widgetHandler:UpdateLayout(lastSecs)
  
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

function UnitCreated(unitID, unitDefID)
  return widgetHandler:UnitCreated(unitID, unitDefID)
end

function UnitReady(unitID, unitDefID, builderID, builderUnitDefID)
  return widgetHandler:UnitReady(unitID, unitDefID, builderID, builderUnitDefID)
end

function UnitDestroyed(unitID, unitDefID)
  return widgetHandler:UnitDestroyed(unitID, unitDefID)
end

function UnitChangedTeam(unitID, unitDefID, oldTeam, newTeam)
  return widgetHandler:UnitChangedTeam(unitID, unitDefID, oldTeam, newTeam)
end


--------------------------------------------------------------------------------

