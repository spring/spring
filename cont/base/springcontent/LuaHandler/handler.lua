--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    handler.lua
--  brief:   the addon (widget/gadget) manager, a call-in router
--  author:  jK (based heavily on code by Dave Rodgers)
--
--  Copyright (C) 2007-2011.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--FIXME name widgets & gadgets AddOns internally

--// Note: all here included modules/utilities are auto exposed to the addons, too!
require "setupdefs.lua"
require "savetable.lua"
require "keysym.lua"
require "actions.lua"


--// make a copy of the engine exported enviroment (we use this later for the addons!)
local EG = {}
for i,v in pairs(_G) do
	EG[i] = v
end

--// don't auto expose the following the addons
require "list.lua"
require "table.lua"
require "VFS_GetFileChecksum.lua"


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- SpeedUp & Helpers

local type   = type
local pcall  = pcall
local pairs  = pairs
local ipairs = ipairs
local emptyTable = {}


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- A Lua List Object

local function SortAddonsFunc(ki1, ki2)
	local one_before_two = tfind(ki1.before, ki2.name) or tfind(ki2.after, ki1.name)
	local two_before_one = tfind(ki2.before, ki1.name) or tfind(ki1.after, ki2.name)

	local one_before_all = tfind(ki1.before, "all")
	local two_before_all = tfind(ki2.before, "all")
	if (one_before_all ~= two_before_all) then
		return one_before_all
	end

	if (ki1.api ~= ki2.api) then
		return (ki1.api)
	end

	local l1 = ki1.layer or math.huge
	local l2 = ki2.layer or math.huge
	if (l1 ~= l2) then
		return (l1 < l2)
	end

	local o1 = handler.orderList[n1] or math.huge
	local o2 = handler.orderList[n2] or math.huge
	if (o1 ~= o2) then
		return (o1 < o2)
	end

	if (ki1.fromZip ~= ki2.fromZip) then --// load zip files first, so they can prevent hacks/cheats ...
		return (ki1.fromZip)
	end

	return (ki1.name < ki2.name)
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  the handler object
--

handler = {
	name = "widgetHandler";
	addonName = "widget";

	verbose = true;
	autoUserWidgets = false; --FIXME move to widget

	addons       = CreateList("addons", SortAddonsFunc); --// all loaded addons
	configData   = {};
	orderList    = {};
	knownInfos = {}; --// cached Load...Info() results of all known/available Addons (even unloaded ones)
	knownChanged = 0;

	commands          = {}; --FIXME where used?
	customCommands    = {};
	inCommandsChanged = false;

	EG = EG;      --// engine global (all published funcs by the engine)
	SG = {};      --// shared table for addons
	globals = {}; --// global vars/funcs

	knownCallIns    = {};
	callInLists     = setmetatable({}, {__index = function(self, key) self[key] = CreateList(key, SortAddonsFunc); return self[key]; end});
	callInHookFuncs = {};

	mouseOwner  = nil;
	initialized = false;
}

handler.AddonName = handler.addonName:gsub("^%l", string.upper) --// widget -> Widget

--// Backwardcompability
handler[handler.addonName .. "s"] = handler.addons  --// handler.widgets == handler.addons


--// backward compability, so you can still call handler:UnitCreated() etc.
setmetatable(handler, {
	__index = function(self, key)
		if (key == "knownWidgets") or (key == "knownAddons") then
			return self.knownInfos
		elseif (key == "actionHandler") then
			return actionHandler.oldSyntax
		end

		if type(key) ~= "string" then
			return nil
		end
		local firstChar = key:sub(1,1)
		if (firstChar == firstChar:upper() and self.knownCallIns[key]) then
			return function(_, ...)
				if (self.callInHookFuncs[key]) then
					return self.callInHookFuncs[key](...)
				else
					error(LUA_NAME .. ": No CallIn-Handler for \"" .. key .. "\"", 2)
				end
			end
		end
	end;
})


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Create list of known CallIns
--

--// always register those callins even when not used by any addon
local staticCallInList = {
	'ConfigureLayout',
	'Shutdown',
	'Update',
}

for _,ciName in ipairs(staticCallInList) do
	staticCallInList[ciName] = true
end


--// Load all known engine callins
local engineCallIns = Script.GetCallInList() --// important!


--// Create list of all known callins (any others used in addons won't work!)
local knownCallIns = handler.knownCallIns
for ciName,ciParams in pairs(engineCallIns) do
	if (not ciParams.unsynced) and ciParams.controller and (not Script.GetSynced()) then
		--// skip synced only events when we are in an unsynced enviroment
	else
		knownCallIns[ciName] = ciParams
	end
end


--// Registers custom (non-engine) callins
function handler:AddNewCallIn(ciName, unsynced, controller)
	if (knownCallIns[ciName]) then
		return
	end
	knownCallIns[ciName] = {unsynced = unsynced, controller = controller, custom = true}
	for _,addon in handler.addons:iter() do
		handler:UpdateAddonCallIn(ciName, addon)
	end
end


local function s(str) return str:gsub("%%{addon}",handler.addonName):gsub("%%{Addon}",handler.AddonName) end

--// Standard Custom CallIns
handler:AddNewCallIn("BlockAddon", true, true)        --// (addon_name, knownInfo) -> bool block
handler:AddNewCallIn("Initialize", true, false)       --// ()
handler:AddNewCallIn("AddonAdded", true, false)       --// (addon_name)
handler:AddNewCallIn("AddonRemoved", true, false)     --// (addon_name, reason) -- reason can either be "crash" | "user" | "auto" | "dependency"
handler:AddNewCallIn("SelectionChanged", true, true)  --// (selection = {unitID1, unitID1}) -> [newSelection]
handler:AddNewCallIn("CommandsChanged", true, false)  --// ()
handler:AddNewCallIn("TextCommand", true, false)      --// ("command") -- renamed ConfigureLayout


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Custom iterator for all known callins

local function knownCallins_iter(addon, key)
	local ciFunc
	repeat
		key = next(knownCallIns, key)
		if (key) then
			ciFunc = addon[key]
			if (type(ciFunc) == "function") then
				return key, ciFunc
			end
		end
	until (not key)
end

local function knownCallins(addon)
	return knownCallins_iter, addon, nil
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Initialize

function handler:Initialize()
	handler:UpdateAddonList()
	handler.initialized = true
end


--FIXME rename (it updates list AND loads them)
function handler:UpdateAddonList()
	handler:LoadOrderList()
	handler:LoadConfigData()
	handler:LoadKnownData()

	--// GetInfo() of new/changed files
	handler:SearchForNew()

	--// Create list all to load files
	Spring.Log(LUA_NAME, "info", ("Loading %ss   <>=vfs  **=raw  ()=unknown"):format(handler.addonName))

	local loadList = {}
	for name,order in pairs(handler.orderList) do
		if (order > 0) then
			local ki = handler.knownInfos[name]
			if ki then
				loadList[#loadList+1] = name
			else
				if (handler.verbose) then Spring.Log(LUA_NAME, "warning", ("Couldn't find a %s named \"%s\""):format(handler.addonName, name)) end
				handler.knownInfos[name] = nil
				handler.orderList[name] = nil
			end
		end
	end

	--// Sort them
	local SortFunc = function(n1, n2)
		local ki1 = handler.knownInfos[n1]
		local ki2 = handler.knownInfos[n2]
		--assert(wi1 and wi2)
		return SortAddonsFunc(ki1 or emptyTable, ki2 or emptyTable)
	end
	table.sort(loadList, SortFunc)

	if (not handler.verbose) then
		--// if not in verbose mode, print the to be load addons (in a nice table) BEFORE loading them!
		local st = {}
		for _,name in ipairs(loadList) do
			st[#st+1] = handler:GetFancyString(name)
		end
		tprinttable(st, 4)
	end

	--// Load them
	for _,name in ipairs(loadList) do
		local ki = handler.knownInfos[name]
		handler:Load(ki.filepath)
	end

	--// Save the active addons, and their ordering
	handler:SaveOrderList()
	handler:SaveConfigData()
	handler:SaveKnownData()
end

handler[("Update%sList"):format(handler.AddonName)] = handler.UpdateAddonList


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Addon Files Finder

function handler:SetVFSMode(newmode)
	VFSMODE = newmode
end


local function GetAllAddonFiles(quiet)
	local spLog = ((quiet) and function() end) or Spring.Log
	local addonFiles = {}
	for i,dir in pairs(ADDON_DIRS) do
		spLog(LUA_NAME, "info", "Scanning: " .. dir)
		local files = VFS.DirList(dir, "*.lua", VFSMODE)
		if (files) then
			tappend(addonFiles, files)
		end
	end
	return addonFiles
end


function handler:FindNameByPath(path)
	for _,ki in pairs(handler.knownInfos) do
		if (ki.filepath == path) then
			return ki.name
		end
	end
end


function handler:SearchForNew(quiet)
	local spLog = ((quiet) and function() end) or Spring.Log
	spLog(LUA_NAME, "info", "Searching for new " .. handler.AddonName .. "s")

	local addonFiles = GetAllAddonFiles(quiet)
	for _,fpath in ipairs(addonFiles) do
		local name = handler:FindNameByPath(fpath)
		local ki = name and handler.knownInfos[name]

		if ki and ((not handler.initialized) or ((ki._rev >= 2) and (not ki.active))) then --// don't override the knownInfos[name] of _loaded_ addons!
			if ki and ki.checksum then --// rev2 addons don't save a checksum!
				local checksum = VFS.GetFileChecksum(fpath, VFSMODE)
				if (checksum and (ki.checksum ~= checksum)) then
					ki = nil
				end
			else
				ki = nil
			end
		end

		if (not ki) then
			ki = handler:LoadAddonInfo(fpath)

			if (ki) then
				if (handler.verbose and ki._rev >= 2) then
					spLog(LUA_NAME, "info", ("Found new %s \"%s\""):format(handler.addonName, ki.name))
				end
			end
		end
	end

	handler:DetectEnabledAddons()
end


function handler:DetectEnabledAddons()
	for i,ki in pairs(handler.knownInfos) do
		if (not ki.active) then
			--// default enabled?
			local defEnabled = ki.enabled

			--// enabled or not?
			local order = handler.orderList[ki.name]
			if ((order or 0) > 0)
				or ((order == nil) and defEnabled)
			then
				--// this will be an active addon
				handler.orderList[ki.name] = order or 1235 --// back of the pack for unknown order

				--// we don't auto start addons when just updating the available list
				ki.active = (not handler.initialized)
			else
				--// deactive the addon
				handler.orderList[ki.name] = 0
				ki.active = false
			end
		end
	end
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

require "crashHandler.lua"

--// so addons can use it, too
handler[("SafeCall%s"):format(handler.AddonName)] = SafeCallAddon
handler.SafeCallAddon = SafeCallAddon

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Addon Creation

require "addonRevisions.lua"

function handler:NewAddon(rev)
	return (rev > 1) and AddonRevs.NewAddonRev2() or AddonRevs.NewAddonRev1()
end


function handler:ValidateAddon(addon)
	if (addon.GetTooltip and not addon.IsAbove) then
		return ("%s has GetTooltip() but not IsAbove()"):format(handler.AddonName)
	end
	return nil
end


function handler:LoadAddonInfo(filepath, _VFSMODE)
	_VFSMODE = _VFSMODE or VFSMODE

	--// update so addons can see if something got changed
	handler.knownChanged = handler.knownChanged + 1

	--// clear old knownInfo
	local name = handler:FindNameByPath(filepath)
	if (name) then
		handler.knownInfos[name] = nil --FIXME addon._info and handler.knownInfos[name] point should point to the same table?
	end

	local ki = handler.knownInfos[name]
	if ki and ki.active then
		--// loaded and running, don't override existing knownInfos!
		return
	end

	local err, ki = AddonRevs.LoadAddonInfoRev2(filepath, _VFSMODE)
	if (err == true) then
		return nil --// addon asked for a silent death
	end
	if (not ki) then
		--// try to load it as rev1 addon
		err, ki = AddonRevs.LoadAddonInfoRev1(filepath, _VFSMODE)
	end

	--// fail
	if (not ki) then
		if (err) then Spring.Log(LUA_NAME, "warning", err) end
		return nil
	end

	--// create checksum for rev1 addons
	if (ki._rev <= 1) then
		ki.checksum = VFS.GetFileChecksum(ki.filepath, _VFSMODE or VFSMODE)
	end

	--// check if it's loaded from a zip (game or map)
	ki.fromZip = true
	if (_VFSMODE == VFS.ZIP)or(_VFSMODE == VFS.ZIP_FIRST) then
		ki.fromZip = VFS.FileExists(ki.filepath,VFS.ZIP_ONLY)
	else
		ki.fromZip = not VFS.FileExists(ki.filepath,VFS.RAW_ONLY)
	end

	--// causality
	tappend(ki.after, ki.depend)

	--// validate
	err = AddonRevs.ValidateKnownInfo(ki, _VFSMODE)
	if (err) then
		Spring.Log(LUA_NAME, "warning", err)
		return nil
	end

	--// Blocked Addon?
	if handler:BlockAddon(ki.name, ki) then --FIXME make ki write protected!!!
		ki.blocked = true
	end

	tmergein(handler.knownInfos[ki.name], ki)
	return handler.knownInfos[ki.name]
end


function handler:ParseAddon(ki, filepath, _VFSMODE)
	return AddonRevs.ParseAddon((ki._rev or 0), filepath, _VFSMODE)
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function handler:GetFancyString(name, str)
	if not str then str = name end
	local ki = handler.knownInfos[name]
	if ki then
		if ki.fromZip then
			return ("<%s>"):format(str)
		else
			return ("*%s*"):format(str)
		end
	else
		return ("(%s)"):format(str)
	end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Callin Closures

local function InsertAddonCallIn(ciName, addon)
	local ret = false

	if (knownCallIns[ciName]) then
		local f = addon[ciName]

		--// use callInName__ to respect when a addon dislinked the function via :RemoveAddonCallIn (and there is is still a func named addon[callInName])
		addon[ciName .. "__"] = f --// non closure!

		if ((addon._info._rev or 0) <= 1) then
			--// old addons had addon:CallInXYZ, so we need to pass the addon as self object
			local f_ = f
			f = function(...) return f_(addon, ...) end
		end

		local swf = SafeWrapFunc(addon, f, ciName)

		assert(type(handler.callInLists[ciName]) == "table", "[InsertAddonCallIn][pre]")
		ret = handler.callInLists[ciName]:Insert(addon, swf)
		assert(type(handler.callInLists[ciName]) == "table", "[InsertAddonCallIn][post]")
	elseif (handler.verbose) then
		Spring.Log(LUA_NAME, "warning", "::InsertAddonCallIn: Unknown CallIn \"" .. ciName.. "\"")
	end

	return ret
end


local function RemoveAddonCallIn(ciName, addon)
	local ret = false

	if (knownCallIns[ciName]) then
		addon[ciName .. "__"] = nil

		-- note: redundant lookup, RemoveAddonCallIns can just pass in the ciList
		assert(type(handler.callInLists[ciName]) == "table", "[RemoveAddonCallIn][pre]")
		ret = handler.callInLists[ciName]:Remove(addon)
		assert(type(handler.callInLists[ciName]) == "table", "[RemoveAddonCallIn][post]")
	elseif (handler.verbose) then
		Spring.Log(LUA_NAME, "warning", "::RemoveAddonCallIn: Unknown CallIn \"" .. ciName.. "\"")
	end

	return ret
end


local function RemoveAddonCallIns(addon)
	assert(type(handler.callInLists) == "table")

	for ciName,ciList in pairs(handler.callInLists) do
		assert(type(ciList) == "table", "[RemoveAddonCallIns] pre-removing callin " .. ciName .. " from addon " .. addon._info.name)
		handler:RemoveAddonCallIn(ciName, addon)
		assert(type(ciList) == "table", "[RemoveAddonCallIns] post-removing callin " .. ciName .. " from addon " .. addon._info.name)
	end

	assert(type(handler.callInLists) == "table")
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function handler:Load(filepath, _VFSMODE)
	--// Load KnownInfo
	local ki = handler:LoadAddonInfo(filepath, _VFSMODE)
	if (not ki) then
		return
	end

	if (ki.blocked) then
		--// blocked
		Spring.Log(LUA_NAME, "warning", ("blocked %s \"%s\"."):format(handler.AddonName, ki.name))
		return
	elseif (ki.active) then
		--// Already loaded?
		Spring.Log(LUA_NAME, "warning", ("%s \"%s\" already loaded."):format(handler.AddonName, ki.name))
		return
	end

	--// check dependencies
	for i=1,#ki.depend do
		local dep = ki.depend[i]
		if not (handler.knownInfos[dep] or {}).active then
			Spring.Log(LUA_NAME, "warning", ("Missing/Unloaded dependency \"%s\" for \"%s\"."):format(dep, ki.name))
			return
		end
	end

	--// Load Addon
	local addon = handler:ParseAddon(ki, filepath, _VFSMODE)
	if (not addon) then
		return
	end

	--// Link KnownInfo with addon
	local mt = {
		__index = ki,
		__newindex = function() error("_info tables are read-only",2) end,
		__metatable = "protected"
	}
	addon._info = setmetatable({}, mt)
	addon.whInfo = addon._info --//backward compability

	--// Verbose
	local name = addon._info.name
	local basename = addon._info.basename

	if (handler.verbose or handler.initialized) then
		local loadingstr = ((ki.api and "Loading API %s: ") or "Loading %s: "):format(handler.addonName)
		Spring.Log(LUA_NAME, "info", ("%-20s %-21s  %s"):format(loadingstr, name, handler:GetFancyString(name,basename)))
	end

	--// Add to handler
	ki.active = true
	handler.addons:Insert(addon, addon)

	--// Unsafe addon (don't use pcall for callins)
	if (SAFEWRAP == 1)and(addon._info.unsafe) then
		Spring.Log(LUA_NAME, "warning", ('loaded unsafe %s: %s'):format(handler.addonName, name))
	end

	--// Link the CallIns
	for ciName,ciFunc in knownCallins(addon) do
		InsertAddonCallIn(ciName, addon)
	end
	handler:UpdateCallIns()

	--// Raw access to the handler
	--// rev1 addons normally only got a 'faked' widgetHandler (see :NewAddonRev1)
	--// via the code below they get access to the real one (if they want)
	if (ki.handler and (ki._rev < 2)) then
		addon.handler = handler
		addon.widgetHandler = handler
	end

	--// Initialize the addon
	if (addon.Initialize) then
		SafeCallAddon(addon, "Initialize")
	end

	--// Load the config data
	local config = handler.configData[name]
	if (addon.SetConfigData and config) then
		SafeCallAddon(addon, "SetConfigData", config)
	end

	--// inform other addons
	handler:AddonAdded(name)
end


function handler:Remove(addon, _reason)
	if not addon then
		addon = getfenv(2)
	end

	if (type(addon) ~= "table")or(type(addon._info) ~= "table")or(not addon._info.name) then
		error("Wrong input to handler:Remove()", 2)
	end

	--// Try clean exit
	local name = addon._info.name
	local ki = handler.knownInfos[name]
	if (not ki.active) then
		return
	end
	ki.active = false
	handler:SaveAddonConfigData(addon)
	if (addon.Shutdown) then
		local ok, err = pcall(addon.Shutdown, addon)
		if not ok then
			Spring.Log(LUA_NAME, "error", 'In Shutdown(): ' .. tostring(err))
		end
	end

	--// Remove any links in the handler
	handler:RemoveAddonGlobals(addon)
	actionHandler.RemoveAddonActions(addon)
	handler.addons:Remove(addon)
	RemoveAddonCallIns(addon)

	--// check dependencies
	local rem = {}
	for _,ao in handler.addons:iter() do
		if tfind(ao._info.depend, name) then
			rem[#rem+1] = ao
		end
	end
	for i=1,#rem do
		local ki2 = rem[i]._info
		Spring.Log(LUA_NAME, "info", ("Removed %s:  %-21s  %s (dependent of \"%s\")"):format(handler.addonName, ki2.name, handler:GetFancyString(ki2.name,ki2.basename),name))
		handler:Remove(rem[i], "dependency")
	end

	--// inform other addons
	handler:AddonRemoved(name, _reason or "user")
end

--// backward compab.
handler[s"Load%{Addon}"]   = handler.Load
handler[s"Remove%{Addon}"] = handler.Remove


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Save/Load addon related data

local function LoadConfigFile(filename, envParam)
	if not VFS.FileExists (filename) then
		return {}
	end

	local success, rvalue = pcall(VFS.Include, filename, envParam)
	if (not success) then
		Spring.Log(LUA_NAME, "warning", 'Failed to load: ' .. filename .. '  (' .. tostring(rvalue) .. ')')
	elseif (type(rvalue) ~= "table") then
		Spring.Log(LUA_NAME, "warning", 'Failed to load: ' .. filename .. '  (table data expected, got ' .. type(rvalue) .. ')')
	else
		return rvalue
	end

	return {}
end

function handler:LoadOrderList()
	if (LUA_NAME ~= "LuaUI" and LUA_NAME ~= "LuaMenu") then return end

	handler.orderList = LoadConfigFile(ORDER_FILENAME, {math = {huge = math.huge}})
end


function handler:SaveOrderList()
	if (LUA_NAME ~= "LuaUI" and LUA_NAME ~= "LuaMenu") then return end

	--// update the current order
	local i = 1
	for _,addon in handler.addons:iter() do
		handler.orderList[addon._info.name] = i
		i = i + 1
	end
	table.save(handler.orderList, ORDER_FILENAME, '-- Addon Order List  (0 disables an addon)')
end


function handler:LoadKnownData()
	if (LUA_NAME ~= "LuaUI" and LUA_NAME ~= "LuaMenu") then return end

	if (handler.initialized) then --FIXME make the code below work even at runtime
		error("Called handler:LoadKnownData after Initialization.", 2)
	end

	handler.knownInfos = LoadConfigFile(KNOWN_FILENAME, {math = {huge = math.huge}})

	for i,ki in pairs(handler.knownInfos) do
		ki.active = nil

		--// Remove non-existing entries
		if not VFS.FileExists(ki.filepath or "", (ki.fromZip and VFS.ZIP_ONLY) or VFS.RAW_ONLY) then
			handler.knownInfos[i] = nil
		end
	end
end


function handler:SaveKnownData()
	if (LUA_NAME ~= "LuaUI" and LUA_NAME ~= "LuaMenu") then return end

	local t = {}
	for i,ki in pairs(handler.knownInfos) do
		if ((ki._rev or 0) <= 1) then --// Don't save/cache rev2 addons (there is no safety problem to get their info)
			t[i] = ki
		end
	end
	table.save(t, KNOWN_FILENAME, '-- Filenames -> AddonNames Translation Table')
end


function handler:LoadConfigData()
	if (LUA_NAME ~= "LuaUI" and LUA_NAME ~= "LuaMenu") then return end

	handler.configData = LoadConfigFile(CONFIG_FILENAME, {})
end


function handler:SaveAddonConfigData(addon)
	if (LUA_NAME ~= "LuaUI" and LUA_NAME ~= "LuaMenu") then return end

	if (addon.GetConfigData) then
		local name = addon._info.name
		handler.configData[name] = SafeCallAddon(addon, "GetConfigData")
	end
end


function handler:SaveConfigData()
	if (LUA_NAME ~= "LuaUI" and LUA_NAME ~= "LuaMenu") then return end

	handler:LoadConfigData()
	for _,addon in handler.addons:iter() do
		handler:SaveAddonConfigData(addon)
	end
	table.save(handler.configData, CONFIG_FILENAME, '-- Addon Custom Data')
end


function handler:SendConfigData()
	handler:LoadConfigData()
	for _,addon in handler.addons:iter() do
		local data = handler.configData[addon._info.name]
		if (addon.SetConfigData and data) then
			SafeCallAddon(addon, "SetConfigData", data)
		end
	end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Enable/Disable/Toggle Addons

function handler:FindByName(name)
	for _,addon in handler.addons:iter() do
		if (addon._info.name == name) then
			return addon
		end
	end
end


function handler:Enable(name)
	local ki = handler.knownInfos[name]
	if (not ki) then
		Spring.Log(LUA_NAME, "warning", "::Enable: Couldn\'t find \"" .. name .. "\".")
		return false
	end
	if (ki.active) then
		return false
	end

	local order = handler.orderList[name]
	if ((order or 0) <= 0) then
		handler.orderList[name] = 1
	end
	handler:Load(ki.filepath)
	handler:SaveOrderList()
	return true
end


function handler:Disable(name)
	local ki = handler.knownInfos[name]
	if (not ki) then
		Spring.Log(LUA_NAME, "warning", "::Disable: Didn\'t find \"" .. name .. "\".")
		return false
	end
	if (not ki.active)and((order or 0) > 0) then
		return false
	end

	local addon = handler:FindByName(name)
	if (addon) then
		local str = ((ki.api and "Removing API %s: ") or "Removing %s: "):format(handler.addonName)
		Spring.Echo(("%-20s %-21s  %s"):format(str, name, handler:GetFancyString(name,ki.basename)))
		handler:Remove(addon) --// deactivate
		handler.orderList[name] = 0 --// disable
		handler:SaveOrderList()
		return true
	else
		Spring.Log(LUA_NAME, "warning", "::Disable: Didn\'t find \"" .. name .. "\".")
	end
end


function handler:Toggle(name)
	local ki = handler.knownInfos[name]
	if (not ki) then
		Spring.Log(LUA_NAME, "warning", "::Toggle: Couldn\'t find \"" .. name .. "\".")
		return
	end

	--// we don't want to crash the calling addon, so run in a pcall (FIXME make other/all code safe too?)
	local status, msg = pcall(function()
		if (ki.active) then
			return handler:Disable(name)
		elseif (handler.orderList[name] <= 0) then
			return handler:Enable(name)
		else
			--// the addon is not active, but enabled; disable it
			handler.orderList[name] = 0
			handler:SaveOrderList()
		end
	end)
	if not status then
		Spring.Log(LUA_NAME, "warning", "::Toggle Error: \"" .. msg .. "\".")
		return
	end
	return status
end

--// backward compab.
handler[s"Find%{Addon}"]    = handler.FindByName
handler[s"Enable%{Addon}"]  = handler.Enable
handler[s"Disable%{Addon}"] = handler.Disable
handler[s"Toggle%{Addon}"]  = handler.Toggle

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--  Global var/func management

function handler:RegisterGlobal(owner, name, value)
	if (name == nil)        or
	   (_G[name])           or
	   (handler.globals[name]) or
	   (engineCallIns[name])
	then
		return false
	end
	_G[name] = value
	handler.globals[name] = owner
	return true
end


function handler:DeregisterGlobal(owner, name)
	if ((name == nil) or (handler.globals[name] and (handler.globals[name] ~= owner))) then
		return false
	end
	_G[name] = nil
	handler.globals[name] = nil
	return true
end


function handler:SetGlobal(owner, name, value)
	if ((name == nil) or (handler.globals[name] ~= owner)) then
		return false
	end
	_G[name] = value
	return true
end


function handler:RemoveAddonGlobals(owner)
	local count = 0
	for name, o in pairs(handler.globals) do
		if (o == owner) then
			_G[name] = nil
			handler.globals[name] = nil
			count = count + 1
		end
	end
	return count
end

handler[s"Remove%{Addon}Globals"] = handler.RemoveAddonGlobals

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- CallIn Manager Functions

local function CreateHookFunc(ciName)
	local ciList = handler.callInLists[ciName]
	if (ciName:sub(1, 4) ~= 'Draw')and(ciName:sub(1, 9) ~= 'TweakDraw') then
		return function(...)
			for _,f in ciList:iter(it) do
				f(...)
			end
		end
	else
		return function(...)
			for _,f in ciList:rev_iter(it) do
				f(...)
			end
		end
	end
end


function handler:UpdateCallIn(ciName)
	--// known callin?
	if (not handler.knownCallIns[ciName]) then
		return
	end

	--// always create the hook functions, so addons can use them via e.g. handler:GetTooltip(x,y)
	local hookfunc = handler.callInHookFuncs[ciName]
	if (not hookfunc) then
		hookfunc = CreateHookFunc(ciName)
		handler.callInHookFuncs[ciName] = hookfunc
	end

	--// non-engine callins don't need to be exported to the engine
	if (not engineCallIns[ciName]) then
		return
	end

	local ciList = handler.callInLists[ciName]
	assert(type(ciList) == "table")

	if ((ciList.first) or
	    (staticCallInList[ciName]) or
	    ((ciName == 'GotChatMsg')     and actionHandler.HaveChatAction()) or  --FIXME these are LuaRules only
	    ((ciName == 'RecvFromSynced') and actionHandler.HaveSyncAction()))    --FIXME these are LuaRules only
	then
		--// already exists?
		if (type(_G[ciName]) == "function") then
			return
		end

		--// always assign these call-ins
		_G[ciName] = hookfunc
	else
		_G[ciName] = nil
	end
	Script.UpdateCallIn(ciName)
end


function handler:UpdateAddonCallIn(name, addon)
	local func = addon[name]
	local result = false
	if (type(func) == 'function') then
		result = InsertAddonCallIn(name, addon)
	else
		result = RemoveAddonCallIn(name, addon)
	end
	if result then
		handler:UpdateCallIn(name)
	end
end


function handler:RemoveAddonCallIn(name, addon)
	if RemoveAddonCallIn(name, addon) then
		handler:UpdateCallIn(name)
	end
end


function handler:UpdateCallIns()
	for ciName in pairs(knownCallIns) do
		handler:UpdateCallIn(ciName)
	end
end

handler[s"Remove%{Addon}CallIn"] = handler.RemoveAddonCallIn
handler[s"Update%{Addon}CallIn"] = handler.UpdateAddonCallIn

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--  Some CallIns need custom handlers

require "specialCallinHandlers.lua"

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Start

--// Load game's configfile for this handler
if VFS.FileExists(LUA_DIRNAME .. "config.lua") then
	include("config.lua", nil, VFS.ZIP_FIRST)
else
	include("LuaHandler/config.lua", nil, VFS.ZIP_FIRST)
end

handler:Initialize()

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
