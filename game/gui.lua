--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    gui.lua
--  brief:   entry point for the user LuaUI, can redirect to a mod's LuaUI
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

LUAUI_VERSION = "LuaUI v0.2"

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

if (gl) then
  Spring.Draw = gl  --  backwards compatibility, 0.74b3 did not have 'gl'
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

LUAUI_DIRNAME   = 'LuaUI/'
do
  -- use a versionned directory name if it exists
  local sansslash = string.sub(LUAUI_DIRNAME, 1, -2)
  local versiondir = sansslash .. '-' .. Game.version .. '/'
  local f = io.open(versiondir  .. 'main.lua')
  if (f) then
    f:close()
    LUAUI_DIRNAME = versiondir
  end
end
print('Using LUAUI_DIRNAME = ' .. LUAUI_DIRNAME)


USER_FILENAME   = LUAUI_DIRNAME .. 'main.lua'
CHOOSE_FILENAME = LUAUI_DIRNAME .. 'modui_dialog.lua'
PERM_FILENAME   = LUAUI_DIRNAME .. 'Config/modui_list.lua'

MODUI_DIRNAME   = 'ModUI/'  --  should version this too, exceptions?
MOD_FILENAME    = MODUI_DIRNAME .. 'main.lua'


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function CleanNameSpace()
	MOD_FILENAME    = nil
	USER_FILENAME   = nil
	PERM_FILENAME   = nil
	CHOOSE_FILENAME = nil
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
-- get the mod's GUI permission state
--

local loadFromMod = nil

do
  -- always bring up the mod dialog if CTRL is pressed
  local alt,ctrl,meta,shift = Spring.GetModKeyState()
  if (not ctrl) then  
    local chunk, err = loadfile(PERM_FILENAME)
    if (chunk ~= nil) then
      local tmp = {}
      setfenv(chunk, tmp)
      local perms = chunk()
      if (perms) then
        loadFromMod = perms[Game.modName]
      end
    end
  end
end


--------------------------------------------------------------------------------

local modText

if (loadFromMod ~= false) then
  modText = Spring.LoadTextVFS(MOD_FILENAME)
  if (modText == nil) then
    loadFromMod = false
  end
end

if (loadFromMod == nil) then
  -- setup the mod selection UI
  local chunk, err = loadfile(CHOOSE_FILENAME)
  if (chunk == nil) then
    Spring.Echo('Failed to load ' .. MOD_FILENAME .. ': (' .. err .. ')')
  else
  	CleanNameSpace()
    chunk()
    return
  end
elseif (loadFromMod) then
  -- load the mod's UI
  local chunk, err = loadstring(modText)
  if (chunk == nil) then
    Spring.Echo('Failed to load ' .. MOD_FILENAME .. ': (' .. err .. ')')
  else
    CleanNameSpace()
    chunk()
    return
  end
end


-------------------------------------------------------------------------------- 
-------------------------------------------------------------------------------- 
--
-- load the user's UI
--

do
  local chunk, err = loadfile(USER_FILENAME)
  if (chunk == nil) then
    KillScript('Failed to load ' .. USER_FILENAME .. ' (' .. err .. ')')
  else
    CleanNameSpace()
    chunk()
    return
  end
end


-------------------------------------------------------------------------------- 
-------------------------------------------------------------------------------- 
