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


--  one-shots
local printUpdate          = true
local printDrawWorld       = true
local printDrawScreen      = true
local printTweakDrawScreen = true
local printIsAbove         = true
local printTweakIsAbove    = true


function widget:TextCommand(command)
  if (command ~= "debug") then
    print('DEBUG(TextCommand) '..command)
    return false
  end
  printUpdate          = true
  printDrawWorld       = true
  printDrawScreen      = true
  printTweakDrawScreen = true
  printIsAbove         = true
  printTweakIsAbove    = true
  Debug()
  return true
end


--------------------------------------------------------------------------------

function widget:Initialize()
  print('DEBUG (Initialize)')
  return
end


function widget:Shutdown()
  print('DEBUG (Shutdown)')
  return
end


function widget:GamePreload()
  print('DEBUG (GamePreload)')
  return
end


function widget:GameStart()
  print('DEBUG (GameStart)')
  return
end


function widget:GameOver()
  print('DEBUG (GameOver)')
  return
end


function widget:Update(deltaTime)
  if (printUpdate) then
    printUpdate = false
    print('DEBUG (Update) '..deltaTime)
  end
  return
end


function widget:CommandNotify(id, params, options)
  print('DEBUG (CommandNotify) '..id)
  return false
end


function widget:AddConsoleLine(msg, priority)
  print('DEBUG (AddConsoleLine) '..msg)
  return
end


function widget:ViewResize(vsx, vsy)
  print('DEBUG (ViewResize) '..vsx..' '..vsy)
  return
end


function widget:DrawWorld()
  if (printDrawWorld) then
    printDrawWorld = false
    print('DEBUG (DrawWorld)')
  end
  return
end


function widget:DrawScreen()
  if (printDrawScreen) then
    printDrawScreen = false
    print('DEBUG (DrawScreen)')
  end
  return
end


function widget:KeyPress(key, mods, isRepeat, label, unicode)
  print('DEBUG (KeyPress) ' .. key .. ' ' .. label .. ' ' .. tostring(unicode))
  return false
end


function widget:KeyRelease(key, mods, label, unicode)
  print('DEBUG (KeyReleased) ' .. key .. ' ' .. label .. ' ' .. tostring(unicode))
  return false
end


function widget:MousePress(x, y, button)
  print('DEBUG (MousePress) '..x..' '..y..' '..button)
  return false
end


function widget:MouseMove(x, y, dx, dy, button)
  print('DEBUG (MouseMove) '..x..' '..y..' '..dx..' '..dy..' '..button)
  return false
end


function widget:MouseRelease(x, y, button)
  print('DEBUG (MouseRelease) '..x..' '..y..' '..button)
  return -1
end


function widget:IsAbove(x, y)
  if (printIsAbove) then
    printIsAbove = false
    print('DEBUG (IsAbove) '..x..' '..y)
  end
  return false
end


function widget:GetTooltip(x, y)
  print('DEBUG (GetTooltip) '..x..' '..y)
  return ""
end


function widget:GroupChanged(groupID)
  print('DEBUG (GroupChanged) '..groupID)
  return
end


function widget:CommandsChanged()
  print('DEBUG (CommandsChanged) ')
  return
end


function widget:UnitCreated(unitID, unitDefID, unitTeam)
  print('DEBUG (UnitCreated) '..unitID..' '..unitDefID..' '..unitTeam)
  return
end


function widget:UnitFinished(unitID, unitDefID, unitTeam)
  print('DEBUG (UnitFinished) '..unitID..' '..unitDefID..' '..unitTeam)
  return
end


function widget:UnitFromFactory(unitID, unitDefID, unitTeam,
                                factID, factDefID, userOrders)
  print('DEBUG (UnitFromFactory) '
        ..unitID..' '..unitDefID..' '..unitTeam..' '
        ..factID..' '..factDefID..' '..tostring(userOrders))
  return
end


function widget:UnitDestroyed(unitID, unitDefID, unitTeam)
  print('DEBUG (UnitDestroyed) '..unitID..' '..unitDefID..' '..unitTeam)
  return
end


function widget:UnitTaken(unitID, unitDefID, unitTeam, newTeam)
  print('DEBUG (UnitTaken) '
        ..unitID..' '..unitDefID..' '..unitTeam..' '..newTeam)
  return
end


function widget:UnitGiven(unitID, unitDefID, unitTeam, oldTeam)
  print('DEBUG (UnitGiven) '
        ..unitID..' '..unitDefID..' '..unitTeam..' '..oldTeam)
  return
end


function widget:UnitIdle(unitID, unitDefID, unitTeam)
  print('DEBUG (UnitIdle) '..unitID..' '..unitDefID..' '..unitTeam)
  return
end


function widget:UnitDamaged(unitID, unitDefID, unitTeam, damage, paralyzer)
  print('DEBUG (UnitDamaged) '
        ..unitID..' '..unitDefID..' '..unitTeam..' '
        ..damage..' '..tostring(paralyzer))
  return
end


function widget:UnitSeismicPing(x, y, z, strength)
  print('DEBUG (UnitSeismicPing) '..x..' '..y..' '..z..' '..strength)
  Spring.PlaySoundFile('LuaUI/Sounds/message_admin.wav', strength * 0.25, x, y, z)
  return
end


function widget:UnitLoaded(unitID, unitDefID, unitTeam,
                           transportID, transportTeam)
  print('DEBUG (UnitLoaded) '
        ..unitID..' '..unitDefID..' '..unitTeam..' '
        ..transportID..' '..transportTeam)
  return
end


function widget:UnitUnloaded(unitID, unitDefID, unitTeam,
                           transportID, transportTeam)
  print('DEBUG (UnitUnloaded) '
        ..unitID..' '..unitDefID..' '..unitTeam..' '
        ..transportID..' '..transportTeam)
  return
end


function widget:UnitCloaked(unitID, unitDefID, unitTeam)
  print('DEBUG (UnitCloaked) '
        ..unitID..' '..unitDefID..' '..unitTeam)
  return
end


function widget:UnitDecloaked(unitID, unitDefID, unitTeam)
  print('DEBUG (UnitDecloaked) '
        ..unitID..' '..unitDefID..' '..unitTeam)
  return
end


function widget:TweakDrawScreen()
  if (printTweakDrawScreen) then
    printTweakDrawScreen = false
    print('DEBUG (TweakDrawScreen)')
  end
  return
end


function widget:TweakKeyPress(key, mods, isRepeat)
  print('DEBUG (TweakKeyPress) '..key)
  return false
end


function widget:TweakKeyRelease(key, mods)
  print('DEBUG (TweakKeyRelease) '..key)
  return false
end


function widget:TweakMousePress(x, y, button)
  print('DEBUG (TweakMousePress) '..x..' '..y..' '..button)
  return false
end


function widget:TweakMouseMove(x, y, dx, dy, button)
  print('DEBUG (TweakMouseMove) '..x..' '..y..' '..dx..' '..dy..' '..button)
  return false
end


function widget:TweakMouseRelease(x, y, button)
  print('DEBUG (TweakMouseRelease) '..x..' '..y..' '..button)
  return -1
end


function widget:TweakIsAbove(x, y)
  if (printTweakIsAbove) then
    printTweakIsAbove = false
    print('DEBUG (TweakIsAbove) '..x..' '..y)
  end
  return false
end


function widget:TweakGetTooltip(x, y)
  print('DEBUG (TweakGetTooltip) '..x..' '..y)
  return ""
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
