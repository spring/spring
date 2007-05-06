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
  if (VFS.FileExists(versiondir  .. 'main.lua', VFS.RAW_ONLY)) then
    LUAUI_DIRNAME = versiondir
  end
end
print('Using LUAUI_DIRNAME = ' .. LUAUI_DIRNAME)


USER_FILENAME   = LUAUI_DIRNAME .. 'main.lua'
CHOOSE_FILENAME = LUAUI_DIRNAME .. 'modui_dialog.lua'
PERM_FILENAME   = LUAUI_DIRNAME .. 'Config/modui_list.lua'

MODUI_DIRNAME   = 'ModUI/'  --  should version this too, exceptions?
MOD_FILENAME    = MODUI_DIRNAME .. 'main.lua'


VFS.DEF_MODE = VFS.RAW_FIRST


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

local text


if (loadFromMod ~= false) then
  text = VFS.LoadFile(MOD_FILENAME, VFS.ZIP_ONLY)
  if (text == nil) then
    loadFromMod = false
  end
end


if (loadFromMod == nil) then
  -- setup the mod selection UI
  text = VFS.LoadFile(CHOOSE_FILENAME, VFS.RAW_ONLY)
  if (text == nil) then
    Spring.Echo('Failed to load ' .. CHOOSE_FILENAME .. ': (' .. err .. ')')
  else
    local chunk, err = loadstring(text)
    if (chunk == nil) then
      Spring.Echo('Failed to load ' .. CHOOSE_FILENAME .. ': (' .. err .. ')')
    else
      CleanNameSpace()
      chunk()
      return
    end
  end
elseif (loadFromMod) then
  -- load the mod's UI
  local chunk, err = loadstring(text)
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
  text = VFS.LoadFile(USER_FILENAME, VFS.RAW_ONLY)
  if (text == nil) then
    Script.Kill('Failed to load ' .. USER_FILENAME)
  end
  local chunk, err = loadstring(text)
  if (chunk == nil) then
    Script.Kill('Failed to load ' .. USER_FILENAME .. ' (' .. err .. ')')
  else
    CleanNameSpace()
    chunk()
    return
  end
end


-------------------------------------------------------------------------------- 
-------------------------------------------------------------------------------- 
