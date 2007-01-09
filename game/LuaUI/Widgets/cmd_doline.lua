--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    cmd_doline.lua
--  brief:   adds a command to run raw LUA commands from the game console
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "DoLine",
    desc      = "Adds '/luaui doline ...' to run lua commands  (for devs)",
    author    = "trepan",
    date      = "Jan 8, 2007",
    license   = "GNU GPL, v2 or later",
    layer     = 0,
    enabled   = true  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
-- Low level LUA access
--

function widget:TextCommand(command)
  if (string.find(command, 'doline ') ~= 1) then
    return false
  end
  local cmd = string.sub(command, 8)
  local chunk, err = loadstring(cmd, 'doline')
  if (chunk == nil) then
    print('doline error: '..err)
  else
    local success, err = pcall(chunk)
    if (not success) then
      Spring.SendCommands({"echo "..err})
    end
  end
  return true
end


--------------------------------------------------------------------------------
