--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    config.lua
--  brief:   configfile for handler.lua
--  author:  jK
--
--  Copyright (C) 2011-2013.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

--// 0: disabled
--// 1: enabled, but can be overriden by widget.GetInfo().unsafe
--// 2: always enabled
SAFEWRAP = 1
SAFEDRAW = false  --// requires SAFEWRAP to work

--//
VFSMODE = VFS.RAW_FIRST

--// when false, the handler will `compress` some output (e.g. list of started widgets)
handler.verbose = false or true

local function LoadLibraries()
	for _, lib in pairs(VFS.SubDirs("libs/")) do
		local addonDir = lib .. LUA_NAME .. "/widgets"
		if #VFS.DirList(addonDir) > 0 then
			table.insert(ADDON_DIRS, addonDir)
		end
	end
end

if LUA_NAME == "LuaUI" then
	assert(LUAUI_DIRNAME)

	--// Config & Widget Locations
	ORDER_FILENAME  = LUAUI_DIRNAME .. 'Config/' .. Game.modShortName .. '_order.lua'
	CONFIG_FILENAME = LUAUI_DIRNAME .. 'Config/' .. Game.modShortName .. '_data.lua'
	KNOWN_FILENAME  = LUAUI_DIRNAME .. 'Config/' .. Game.modShortName .. '_known.lua'

	ADDON_DIRS     = {
		LUAUI_DIRNAME .. 'Addons/';
		LUAUI_DIRNAME .. 'Widgets/';
		LUAUI_DIRNAME .. 'SystemAddons/';
		LUAUI_DIRNAME .. 'SystemWidgets/';
		LUA_DIRNAME .. 'chili/'; -- DEPRECATED: Scheduled for removal in 103
	}
	--// Create the "LuaUI/Config" directory
	Spring.CreateDir(LUAUI_DIRNAME .. 'Config') --FIXME LuaRules!

	handler:Load(LUAUI_DIRNAME .. "SystemWidgets/BlockUserWidgets.lua" --[[, VFS.ZIP]])
else
	AddonNames = handler.AddonName .. "s/"

	ADDON_DIRS     = {
		LUA_DIRNAME .. 'Addons/';
		LUA_DIRNAME .. AddonNames;
		LUA_DIRNAME .. 'SystemAddons/';
		LUA_DIRNAME .. 'System' .. AddonNames;
		LUA_DIRNAME .. 'chili/'; -- DEPRECATED: Scheduled for removal in 103
	}
end
LoadLibraries()
