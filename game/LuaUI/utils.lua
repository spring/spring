--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    utils.lua
--  brief:   utility routines
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

if (UtilsGuard) then
  return
end
UtilsGuard = true


function Echo(msg)
  Spring.SendCommands({'echo ' .. msg})
end


function Say(msg)
  Spring.SendCommands({'say ' .. msg})
end


function SendCommand(msg)
  Spring.SendCommands({msg})
end


--------------------------------------------------------------------------------
--
--  returns:  basename, dirname
--

function Basename(path)
  return string.gsub(path, "(.*[\\/])(.*)", "%2") or path,
         string.gsub(path, "(.*[\\/])(.*)", "%1")
end

--------------------------------------------------------------------------------


function include(filename, envTable)
  if (string.find(filename, 'h.lua')) then
    filename = 'Headers/' .. filename
  end
  local chunk, err = loadfile(LUAUI_DIRNAME .. filename)
  if (chunk == nil) then
    error(err)
  end
  if (envTable ~= nil) then
    setfenv(chunk, envTable)
  end
  return chunk()
end


function includeVFS(filename, envTable)
  local str = Spring.LoadTextVFS(filename)
  if (not str) then
    error('Could not load: ' .. filename)
  end
  local chunk, err = loadstring(str, 'includeVFS')
  if (chunk == nil) then
    error(err)
  end
  if (envTable ~= nil) then
    setfenv(chunk, envTable)
  end
  return chunk()
end


--------------------------------------------------------------------------------
