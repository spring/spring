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

function gadget:GetInfo()
  return {
    name      = "DoLine",
    desc      = "Adds '.luarules run|urun|echo|uecho ...' to run lua commands",
    author    = "trepan",
    date      = "May 03, 2007",
    license   = "GNU GPL, v2 or later",
    layer     = 0,
    enabled   = true  --  loaded by default?
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function AllowAction(playerID)
  if (playerID ~= 0) then
    Spring.SendMessageToPlayer(playerID, "Must be the host player")
    return false
  end
  if (not Spring.IsCheatingEnabled()) then
    Spring.SendMessageToPlayer(playerID, "Cheating must be enabled")
    return false
  end
  return true
end


local function RunCmd(cmd, line, words, playerID)
  if (not AllowAction(playerID)) then
    return true
  end
  local chunk, err = loadstring(line, "run", _G)
  if (chunk) then
    chunk()
  end
  return true
end


local function EchoCmd(cmd, line, words, playerID)
  if (not AllowAction(playerID)) then
    return true
  end
  local chunk, err = loadstring("return " .. line, "echo", _G)
  if (chunk) then
    Spring.Echo(chunk())
  end
  return true
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function gadget:Initialize()
  if (gadgetHandler:IsSyncedCode()) then
    gadgetHandler:AddChatAction('run',  RunCmd,
      " <string>:  execute synced lua commands"
    )
    gadgetHandler:AddChatAction('echo', EchoCmd,
      " <string>:  execute synced lua commands, print the output"
    )
  else
    gadgetHandler:AddChatAction('urun',  RunCmd,
      " <string>:  execute unsynced lua commands"
    )
    gadgetHandler:AddChatAction('uecho', EchoCmd,
      " <string>:  execute unsynced lua commands, print the output"
    )
  end
end


function gadget:Shutdown()
  if (gadgetHandler:IsSyncedCode()) then
    gadgetHandler:RemoveChatAction('run')
    gadgetHandler:RemoveChatAction('echo')
  else
    gadgetHandler:RemoveChatAction('urun')
    gadgetHandler:RemoveChatAction('uecho')
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
