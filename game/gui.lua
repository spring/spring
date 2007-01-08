--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  GUI.LUA
--

LUAUI_VERSION = "LuaUI v0.1"

LUAUI_DIRNAME   = 'LuaUI/'

USER_FILENAME   = LUAUI_DIRNAME .. 'main.lua'
CHOOSE_FILENAME = LUAUI_DIRNAME .. 'modui_dialog.lua'
PERM_FILENAME   = LUAUI_DIRNAME .. 'Config/modui_list.lua'

MOD_FILENAME = 'ModUI/main.lua'


function Echo(msg)
  Spring.SendCommands({'echo ' .. msg})
end

function CleanNameSpace()
	MOD_FILENAME    = nil
	USER_FILENAME   = nil
	PERM_FILENAME   = nil
	CHOOSE_FILENAME = nil
	Echo            = nil
  CleanNameSpace  = nil
end


--------------------------------------------------------------------------------
--
-- clear the call-ins
--

Shutdown        = nil
LayoutButtons   = nil
UpdateLayout    = nil
ConfigureLayout = nil
CommandNotify   = nil
DrawWorldItems  = nil
DrawScreenItems = nil
KeyPress        = nil
KeyRelease      = nil
MouseMove       = nil
MousePress      = nil
MouseRelease    = nil
IsAbove         = nil
GetTooltip      = nil
AddConsoleLine  = nil
GroupChanged    = nil
UnitCreated     = nil
UnitReady       = nil
UnitDestroyed   = nil
UnitChangedTeam = nil


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
    Echo('Failed to load ' .. MOD_FILENAME .. ': (' .. err .. ')')
  else
  	CleanNameSpace()
    chunk()
    return
  end
elseif (loadFromMod) then
  -- load the mod's UI
  local chunk, err = loadstring(modText)
  if (chunk == nil) then
    Echo('Failed to load ' .. MOD_FILENAME .. ': (' .. err .. ')')
  else
    CleanNameSpace()
    chunk()
    return
  end
end


-------------------------------------------------------------------------------- 
--
-- load the user's UI
--

do
  local chunk, err = loadfile(USER_FILENAME)
  if (chunk == nil) then
    Echo('Failed to load ' .. USER_FILENAME .. ': (' .. err .. ')')
    LayoutIcons = function () return 'disabled' end
  else
    CleanNameSpace()
    chunk()
    return
  end
end


-------------------------------------------------------------------------------- 
-------------------------------------------------------------------------------- 
