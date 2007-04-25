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
    enabled   = false  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
-- Low level LUA access
--

local function RunCmd(cmd, optLine)
  print(optLine)
  local chunk, err = loadstring(optLine, 'run')
  if (chunk == nil) then
    print('doline error: ' .. err)
  else
    local success, err = pcall(chunk)
    if (not success) then
      Spring.Echo(err)
    end	
  end
  return true
end


local function EchoCmd(cmd, optLine)
  local chunk, err = loadstring("return " .. optLine, 'echo')
  if (chunk == nil) then
    print('doline error: ' .. err)
  else
    local results = { pcall(chunk) }
    local success = results[1]
    if (not success) then
      Spring.Echo(results[2])
    else
      table.remove(results, 1)
      Spring.Echo(unpack(results))
    end
  end
  return true
end


function widget:Initialize()
  widgetHandler:AddAction("doline",  RunCmd)  -- backwards compatible
  widgetHandler:AddAction("run",     RunCmd)
  widgetHandler:AddAction("echolua", EchoCmd, nil)
  widgetHandler:AddAction("echo",    EchoCmd, nil, "t") -- text only

  -- NOTE: that last entry is text only so that we do not
  --       override the default "/echo" Spring command when
  --       it is bound to a keyset.
end


--------------------------------------------------------------------------------
