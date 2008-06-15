--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    dbg_debug.lua
--  brief:   a widget that prints debug data
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "Debug",
    desc      = "Adds '/luaui debug' and prints debug info  (for devs)",
    author    = "trepan",
    date      = "Jan 8, 2007",
    license   = "GNU GPL, v2 or later",
    layer     = -10,
    enabled   = false  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Low level LUA access
--

include("debug.lua")


local Echo = Spring.Echo


--  one-shots
local printUpdate          = true
local printDrawWorld       = true
local printDrawScreen      = true
local printTweakDrawScreen = true
local printIsAbove         = true
local printTweakIsAbove    = true
local printAddConsoleLine  = true


function widget:TextCommand(command)
  if (command ~= "debug") then
    Echo('DEBUG(TextCommand) '..command)
    return false
  end
  printUpdate          = true
  printDrawWorld       = true
  printDrawScreen      = true
  printTweakDrawScreen = true
  printIsAbove         = true
  printTweakIsAbove    = true
  printAddConsoleLine  = true
  Debug()
  return true
end


--------------------------------------------------------------------------------

function widget:Initialize()
  Echo('DEBUG (Initialize)')
  return
end


function widget:Shutdown()
  Echo('DEBUG (Shutdown)')
  return
end


function widget:GamePreload()
  Echo('DEBUG (GamePreload)')
  return
end


function widget:GameStart()
  Echo('DEBUG (GameStart)')
  return
end


function widget:GameOver()
  Echo('DEBUG (GameOver)')
  return
end


function widget:Update(deltaTime)
  if (printUpdate) then
    printUpdate = false
    Echo('DEBUG (Update) '..deltaTime)
  end
  return
end


function widget:CommandNotify(id, params, options)
  Echo('DEBUG (CommandNotify) '..id)
  return false
end


function widget:AddConsoleLine(msg, priority)
  if (printAddConsoleLine) then
    Echo('DEBUG (AddConsoleLine) '..msg)
    printAddConsoleLine = false
  end
  return
end


function widget:ViewResize(vsx, vsy)
  Echo('DEBUG (ViewResize) '..vsx..' '..vsy)
  return
end


function widget:DrawWorld()
  if (printDrawWorld) then
    printDrawWorld = false
    Echo('DEBUG (DrawWorld)')
  end
  return
end


function widget:DrawScreen()
  if (printDrawScreen) then
    printDrawScreen = false
    Echo('DEBUG (DrawScreen)')
  end
  return
end


function widget:KeyPress(key, mods, isRepeat, label, unicode)
  Echo('DEBUG (KeyPress) ' .. key .. ' ' .. label .. ' ' .. tostring(unicode))
  return false
end


function widget:KeyRelease(key, mods, label, unicode)
  Echo('DEBUG (KeyReleased) ' .. key .. ' ' .. label .. ' ' .. tostring(unicode))
  return false
end


function widget:MousePress(x, y, button)
  Echo('DEBUG (MousePress) '..x..' '..y..' '..button)
  return false
end


function widget:MouseMove(x, y, dx, dy, button)
  Echo('DEBUG (MouseMove) '..x..' '..y..' '..dx..' '..dy..' '..button)
  return false
end


function widget:MouseRelease(x, y, button)
  Echo('DEBUG (MouseRelease) '..x..' '..y..' '..button)
  return -1
end


function widget:IsAbove(x, y)
  if (printIsAbove) then
    printIsAbove = false
    Echo('DEBUG (IsAbove) '..x..' '..y)
  end
  return false
end


function widget:GetTooltip(x, y)
  Echo('DEBUG (GetTooltip) '..x..' '..y)
  return ""
end


function widget:GroupChanged(groupID)
  Echo('DEBUG (GroupChanged) '..groupID)
  return
end


function widget:CommandsChanged()
  Echo('DEBUG (CommandsChanged) ')
  return
end


function widget:UnitCreated(unitID, unitDefID, unitTeam)
  Echo('DEBUG (UnitCreated) '..unitID..' '..unitDefID..' '..unitTeam)
  return
end


function widget:UnitFinished(unitID, unitDefID, unitTeam)
  Echo('DEBUG (UnitFinished) '..unitID..' '..unitDefID..' '..unitTeam)
  return
end


function widget:UnitFromFactory(unitID, unitDefID, unitTeam,
                                factID, factDefID, userOrders)
  Echo('DEBUG (UnitFromFactory) '
       ..unitID..' '..unitDefID..' '..unitTeam..' '
       ..factID..' '..factDefID..' '..tostring(userOrders))
  return
end


function widget:UnitDestroyed(unitID, unitDefID, unitTeam)
  Echo('DEBUG (UnitDestroyed) '..unitID..' '..unitDefID..' '..unitTeam)
  return
end


function widget:UnitTaken(unitID, unitDefID, unitTeam, newTeam)
  Echo('DEBUG (UnitTaken) '
       ..unitID..' '..unitDefID..' '..unitTeam..' '..newTeam)
  return
end


function widget:UnitGiven(unitID, unitDefID, unitTeam, oldTeam)
  Echo('DEBUG (UnitGiven) '
       ..unitID..' '..unitDefID..' '..unitTeam..' '..oldTeam)
  return
end


function widget:UnitIdle(unitID, unitDefID, unitTeam)
  Echo('DEBUG (UnitIdle) '..unitID..' '..unitDefID..' '..unitTeam)
  return
end


function widget:UnitDamaged(unitID, unitDefID, unitTeam, damage, paralyzer)
  Echo('DEBUG (UnitDamaged) '
       ..unitID..' '..unitDefID..' '..unitTeam..' '
       ..damage..' '..tostring(paralyzer))
  return
end


function widget:UnitSeismicPing(x, y, z, strength)
  Echo('DEBUG (UnitSeismicPing) '..x..' '..y..' '..z..' '..strength)
  Spring.PlaySoundFile('LuaUI/Sounds/message_admin.wav', strength * 0.25, x, y, z)
  return
end


function widget:UnitLoaded(unitID, unitDefID, unitTeam,
                           transportID, transportTeam)
  Echo('DEBUG (UnitLoaded) '
       ..unitID..' '..unitDefID..' '..unitTeam..' '
       ..transportID..' '..transportTeam)
  return
end


function widget:UnitUnloaded(unitID, unitDefID, unitTeam,
                           transportID, transportTeam)
  Echo('DEBUG (UnitUnloaded) '
       ..unitID..' '..unitDefID..' '..unitTeam..' '
       ..transportID..' '..transportTeam)
  return
end


function widget:UnitCloaked(unitID, unitDefID, unitTeam)
  Echo('DEBUG (UnitCloaked) '
       ..unitID..' '..unitDefID..' '..unitTeam)
  return
end


function widget:UnitDecloaked(unitID, unitDefID, unitTeam)
  Echo('DEBUG (UnitDecloaked) '
       ..unitID..' '..unitDefID..' '..unitTeam)
  return
end


function widget:TweakDrawScreen()
  if (printTweakDrawScreen) then
    printTweakDrawScreen = false
    Echo('DEBUG (TweakDrawScreen)')
  end
  return
end


function widget:TweakKeyPress(key, mods, isRepeat)
  Echo('DEBUG (TweakKeyPress) '..key)
  return false
end


function widget:TweakKeyRelease(key, mods)
  Echo('DEBUG (TweakKeyRelease) '..key)
  return false
end


function widget:TweakMousePress(x, y, button)
  Echo('DEBUG (TweakMousePress) '..x..' '..y..' '..button)
  return false
end


function widget:TweakMouseMove(x, y, dx, dy, button)
  Echo('DEBUG (TweakMouseMove) '..x..' '..y..' '..dx..' '..dy..' '..button)
  return false
end


function widget:TweakMouseRelease(x, y, button)
  Echo('DEBUG (TweakMouseRelease) '..x..' '..y..' '..button)
  return -1
end


function widget:TweakIsAbove(x, y)
  if (printTweakIsAbove) then
    printTweakIsAbove = false
    Echo('DEBUG (TweakIsAbove) '..x..' '..y)
  end
  return false
end


function widget:TweakGetTooltip(x, y)
  Echo('DEBUG (TweakGetTooltip) '..x..' '..y)
  return ""
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
