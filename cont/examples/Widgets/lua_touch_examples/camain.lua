-- $Id: camain.lua 3171 2008-11-06 09:06:29Z det $
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

--
-- 0.75b2 compatibilty
--
if (Spring.GetTeamColor == nil) then
  local getTeamInfo = Spring.GetTeamInfo
  Spring.GetTeamColor = function(teamID)
    local _,_,_,_,_,_,r,g,b,a = getTeamInfo(teamID)
    return r, g, b, a
  end
  Spring.GetTeamInfo = function(teamID)
    local id, leader, active, isDead, isAi, side,
          r, g, b, a, allyTeam = getTeamInfo(teamID)
    return id, leader, active, isDead, isAi, side, allyTeam
  end
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

Spring.SendCommands({"ctrlpanel " .. LUAUI_DIRNAME .. "ctrlpanel.txt"})

VFS.Include(LUAUI_DIRNAME .. 'utils.lua', utilFile)

include("setupdefs.lua")
include("savetable.lua")

include("debug.lua")
include("modfonts.lua")
include("layout.lua")   -- contains a simple LayoutButtons()
include("cawidgets.lua")  -- the widget handler

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

local gl = Spring.Draw  --  easier to use


-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
--
--  A few helper functions
--

function Say(msg)
  Spring.SendCommands({'say ' .. msg})
end


-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
--
--  Update()  --  called every frame
--

activePage = 0

forceLayout = true


function Update()
  local currentPage = Spring.GetActivePage()
  if (forceLayout or (currentPage ~= activePage)) then
    Spring.ForceLayoutUpdate()  --  for the page number indicator
    forceLayout = false
  end
  activePage = currentPage

  fontHandler.Update()

  widgetHandler:Update()

  return
end


-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
--
--  WidgetHandler fixed calls
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

function DrawScreen(vsx, vsy)
  return widgetHandler:DrawScreen()
end

function KeyPress(key, mods, isRepeat)
  return widgetHandler:KeyPress(key, mods, isRepeat)
end

function KeyRelease(key, mods)
  return widgetHandler:KeyRelease(key, mods)
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

function AddTouch(id, x, y)
  return widgetHandler:AddCursor(x, y, id)
end

function UpdateTouch(id, x, y, dx, dy)
	widgetHandler:UpdateCursor(x, y, dx, dy, id)
end

function RemoveTouch(id, x, y, dx, dy)
	widgetHandler:RemoveCursor(x, y, dx, dy, id)
end

function RefreshTouches(seconds)
	widgetHandler:RefreshCursors(seconds)
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

local allModOptions = Spring.GetModOptions()
function Spring.GetModOption(s,bool,default)
  if (bool) then
    local modOption = allModOptions[s]
    if (modOption==nil) then modOption = (default and "1") end
    return (modOption=="1")
  else
    local modOption = allModOptions[s]
    if (modOption==nil) then modOption = default end
    return modOption
  end
end

--
-- The unit (and some of the Draw) call-ins are handled
-- differently (see LuaUI/widgets.lua / UpdateCallIns())
--


--------------------------------------------------------------------------------

