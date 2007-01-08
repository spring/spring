--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    cmd_givemobile.lua
--  brief:   adds a command to give all mobile units
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "GiveMobile",
    desc      = "Adds '/luaui give mobile', a limited '.give all'",
    author    = "trepan",
    date      = "Jan 11, 2007",
    license   = "GNU GPL, v2 or later",
    drawLayer = 0,
    enabled   = true  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:TextCommand(command)
  if (command ~= 'give mobile') then
    return false
  end
  Spring.SendCommands({"say .cheat 1"}) -- enable cheating
  for udid,ud in pairs(UnitDefs) do
    if (ud and ud.canMove and (ud.speed > 0)) then
      Spring.SendCommands({"say .give " .. ud.name})
    end
  end
  return true
end


--------------------------------------------------------------------------------
